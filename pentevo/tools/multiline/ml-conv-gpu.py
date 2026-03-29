# ml-conv-opencl-rgb555.py
# Uses original Floyd–Steinberg dithering (copied unchanged from ml-conv.py).
# Performs KMeans per-line in RGB555 color space with GPU assignment (OpenCL).
# Centroid update is on CPU; centers remain in 5-bit domain.

from PIL import Image
import numpy as np
import sys
import pyopencl as cl

# ---------- Original RGB<->RGB555 helpers ----------
def rgb_to_rgb555(color):
    r, g, b = color
    r, g, b = int(r), int(g), int(b)
    r = (r >> 3) & 0x1F
    g = (g >> 3) & 0x1F
    b = (b >> 3) & 0x1F
    return (r, g, b)

def rgb555_to_rgb(color):
    r, g, b = color
    r = (r << 3) | (r >> 2)
    g = (g << 3) | (g >> 2)
    b = (b << 3) | (b >> 2)
    return (r, g, b)

def rgb555_to_16bit(color):
    r, g, b = color
    result = 0x8000 | (np.uint16(r) << 10) | (np.uint16(g) << 5) | (np.uint16(b) << 0)
    return result

# ---------- Exact Floyd–Steinberg dithering (from original script) ----------
def floyd_steinberg_dithering(image):
    pixels = np.array(image, dtype=np.float32)
    height, width, _ = pixels.shape
    dithered_pixels = np.zeros_like(pixels, dtype=np.float32)
    
    for y in range(height):
        for x in range(width):
            old_pixel = pixels[y, x].copy()
            new_pixel = np.floor(old_pixel / 8) * 8
            dithered_pixels[y, x] = new_pixel
            quant_error = old_pixel - new_pixel
            if x + 1 < width:
                pixels[y, x + 1] += quant_error * 7 / 16
            if y + 1 < height:
                if x > 0:
                    pixels[y + 1, x - 1] += quant_error * 3 / 16
                pixels[y + 1, x] += quant_error * 5 / 16
                if x + 1 < width:
                    pixels[y + 1, x + 1] += quant_error * 1 / 16
    
    return np.clip(dithered_pixels, 0, 255).astype(np.uint8)

# ---------- Utility functions ----------
def print_progress_bar(iteration, total, prefix='Progress:', suffix='Complete', decimals=1, length=50, fill='█'):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filled_length = int(length * iteration // total)
    bar = fill * filled_length + '-' * (length - filled_length)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end='')
    if iteration == total:
        print()

def resize_to_360x288(image):
    target_width, target_height = 360, 288
    original_width, original_height = image.size
    aspect_ratio = original_width / original_height

    if aspect_ratio > target_width / target_height:
        new_width = target_width
        new_height = int(target_width / aspect_ratio)
    else:
        new_height = target_height
        new_width = int(target_height * aspect_ratio)

    resized_image = image.resize((new_width, new_height), Image.LANCZOS)
    new_image = Image.new("RGB", (target_width, target_height), (0, 0, 0))
    paste_x = (target_width - new_width) // 2
    paste_y = (target_height - new_height) // 2
    new_image.paste(resized_image, (paste_x, paste_y))
    return new_image

# ---------- OpenCL kernel ----------
KMEANS_ASSIGN_KERNEL = """
__kernel void assign_clusters(
    __global const float *points,
    __global const float *centers,
    __global int *labels,
    const int n_points,
    const int n_clusters)
{
    int gid = get_global_id(0);
    if (gid >= n_points) return;

    float px = points[gid*3 + 0];
    float py = points[gid*3 + 1];
    float pz = points[gid*3 + 2];

    float min_dist = 1e30f;
    int best = 0;

    for (int c = 0; c < n_clusters; c++) {
        float cx = centers[c*3 + 0];
        float cy = centers[c*3 + 1];
        float cz = centers[c*3 + 2];

        float dx = px - cx;
        float dy = py - cy;
        float dz = pz - cz;
        float dist = dx*dx + dy*dy + dz*dz;

        if (dist < min_dist) {
            min_dist = dist;
            best = c;
        }
    }
    labels[gid] = best;
}
"""

# ---------- GPU KMeans (assignment on GPU, update on CPU) ----------
class GPUKMeansRGB555:
    def __init__(self, n_clusters=16, max_iter=30, tol=1e-4, ctx=None):
        self.n_clusters = n_clusters
        self.max_iter = max_iter
        self.tol = tol
        self.ctx = ctx or cl.create_some_context(interactive=False)
        self.queue = cl.CommandQueue(self.ctx)
        self.program = cl.Program(self.ctx, KMEANS_ASSIGN_KERNEL).build()
        # fix: retrieve kernel once
        self.kernel = cl.Kernel(self.program, "assign_clusters")

    def fit_predict(self, points_5bit):
        points = points_5bit.astype(np.float32).copy()
        n_points = points.shape[0]
        K = self.n_clusters
        mf = cl.mem_flags
        rng = np.random.default_rng()

        if n_points <= K:
            labels = np.arange(n_points, dtype=np.int32)
            centers = np.zeros((K, 3), dtype=np.float32)
            centers[:n_points] = points
            for i in range(n_points, K):
                centers[i] = points[rng.integers(0, n_points)]
            labels = np.pad(labels, (0, K - n_points), 'wrap')[:n_points]
            return labels, centers

        unique_idx = rng.choice(n_points, size=K, replace=False)
        centers = points[unique_idx].astype(np.float32).copy()

        buf_points = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=points.ravel().astype(np.float32))
        buf_labels = cl.Buffer(self.ctx, mf.WRITE_ONLY, size=n_points * np.int32().nbytes)
        labels = np.empty(n_points, dtype=np.int32)

        for it in range(self.max_iter):
            buf_centers = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=centers.ravel().astype(np.float32))

            # reuse kernel
            self.kernel.set_args(buf_points, buf_centers, buf_labels, np.int32(n_points), np.int32(K))
            cl.enqueue_nd_range_kernel(self.queue, self.kernel, (n_points,), None)
            cl.enqueue_copy(self.queue, labels, buf_labels)
            self.queue.finish()

            new_centers = np.zeros_like(centers)
            counts = np.zeros((K,), dtype=np.int32)
            for i, lab in enumerate(labels):
                new_centers[lab] += points[i]
                counts[lab] += 1
            for c in range(K):
                if counts[c] > 0:
                    new_centers[c] /= counts[c]
                else:
                    new_centers[c] = points[rng.integers(0, n_points)]

            shift = np.linalg.norm(new_centers - centers)
            centers = new_centers.astype(np.float32)
            if shift < self.tol:
                break

        centers = np.clip(np.rint(centers), 0, 31).astype(np.float32)
        return labels, centers

# ---------- Line reduction ----------
def reduce_line_colors_gpu_rgb555(line_rgb555, max_colors=128):
    h, w, _ = line_rgb555.shape
    pts = line_rgb555.reshape(-1, 3).astype(np.float32)
    unique_colors = np.unique(pts, axis=0)
    if unique_colors.shape[0] <= max_colors:
        return line_rgb555.astype(np.uint8)

    kmeans = GPUKMeansRGB555(n_clusters=max_colors, max_iter=30)
    labels, centers = kmeans.fit_predict(pts)
    centers_u8 = centers.astype(np.uint8)
    new_pts = centers_u8[labels].reshape(h, w, 3)
    return new_pts.astype(np.uint8)

# ---------- Conversion pipeline ----------
def convert_image(input_path, output_path, max_colors=128, use_dither=False, use_interleave=False):
    try:
        img = Image.open(input_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        img = resize_to_360x288(img)
    except Exception as e:
        print(f"Error loading or resizing image: {e}")
        return

    pixels = img if not use_dither else Image.fromarray(floyd_steinberg_dithering(img))
    arr_pixels = np.array(pixels)

    height, width, _ = arr_pixels.shape
    rgb555_pixels = np.zeros((height, width, 3), dtype=np.uint8)
    for y in range(height):
        for x in range(width):
            rgb555_pixels[y, x] = rgb_to_rgb555(arr_pixels[y, x])

    max_colors_used = 0
    for y in range(height):
        line = rgb555_pixels[y:y+1, :, :]
        reduced_line = reduce_line_colors_gpu_rgb555(line, max_colors)
        rgb555_pixels[y:y+1, :, :] = reduced_line
        unique_colors = len(np.unique(reduced_line.reshape(-1, 3), axis=0))
        max_colors_used = max(max_colors_used, unique_colors)
        print_progress_bar(y + 1, height)

    final_pixels = np.zeros((height, width, 3), dtype=np.uint8)
    for y in range(height):
        for x in range(width):
            final_pixels[y, x] = rgb555_to_rgb(tuple(rgb555_pixels[y, x]))

    try:
        result_img = Image.fromarray(final_pixels)
        result_img.save(output_path, 'PNG')
        print(f"\nImage successfully saved to {output_path}")
    except Exception as e:
        print(f"Error saving image: {e}")
        return

    base_name = output_path.rsplit('.', 1)[0]
    pix_path = base_name + '.pix'
    pal_path = base_name + '.pal'
    with open(pix_path, 'wb') as pix_file, open(pal_path, 'wb') as pal_file:
        for y in range(height):
            line = rgb555_pixels[y]
            unique_colors = np.unique(line.reshape(-1, 3), axis=0)
            if len(unique_colors) > max_colors:
                unique_colors = unique_colors[:max_colors]
            palette = np.zeros((max_colors, 3), dtype=np.uint8)
            palette[:len(unique_colors)] = unique_colors
            for color in palette:
                color_16bit = rgb555_to_16bit(tuple(color))
                pal_file.write(bytes([color_16bit & 0xFF, (color_16bit >> 8) & 0xFF]))
            current_offset = max_colors if use_interleave and y % 2 == 1 else 0
            color_to_index = {tuple(color): (i + current_offset) % 256 for i, color in enumerate(unique_colors)}
            for x in range(width):
                pixel = tuple(line[x])
                index = color_to_index.get(pixel, current_offset)
                pix_file.write(bytes([index & 0xFF]))

    print(f"Files .pix and .pal successfully saved: {pix_path}, {pal_path}")
    print(f"Maximum number of colors in a line: {max_colors_used}")

# ---------- CLI ----------
if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) > 7:
        print(f"Usage: {sys.argv[0]} <input> <output.png> [<max_colors>] [-d] [-i]")
    else:
        input_path = sys.argv[1]
        output_path = sys.argv[2]
        max_colors = 128
        use_dither = False
        use_interleave = False
        args = sys.argv[3:]
        i = 0
        while i < len(args):
            if args[i] == "-d":
                use_dither = True
            elif args[i] == "-i":
                use_interleave = True
            elif args[i].isdigit():
                try:
                    max_colors = int(args[i])
                    if max_colors <= 0:
                        raise ValueError
                except ValueError:
                    print("Error: max_colors must be a positive integer")
                    sys.exit(1)
            i += 1
        print(f"Parameters: input_path={input_path}, output_path={output_path}, max_colors={max_colors}, use_dither={use_dither}, use_interleave={use_interleave}")
        convert_image(input_path, output_path, max_colors, use_dither, use_interleave)
