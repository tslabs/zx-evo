from PIL import Image
import numpy as np
from sklearn.cluster import KMeans
import sys

def rgb_to_rgb555(color):
    """Converts RGB (8 bits per component) to RGB555 (5 bits per component)."""
    r, g, b = color
    r, g, b = int(r), int(g), int(b)
    r = (r >> 3) & 0x1F
    g = (g >> 3) & 0x1F
    b = (b >> 3) & 0x1F
    return (r, g, b)

def rgb555_to_rgb(color):
    """Converts RGB555 back to RGB for saving."""
    r, g, b = color
    r = (r << 3) | (r >> 2)
    g = (g << 3) | (g >> 2)
    b = (b << 3) | (b >> 2)
    return (r, g, b)

def rgb555_to_16bit(color):
    """Converts RGB555 to 16-bit format 1RRRRRGG GGGBBBBB."""
    r, g, b = color
    result = 0x8000 | (np.uint16(r) << 10) | (np.uint16(g) << 5) | (np.uint16(b) << 0)
    return result

def floyd_steinberg_dithering(image):
    """Applies Floyd-Steinberg dithering with discarding 3 least significant bits for RGB555."""
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

def reduce_line_colors(line, max_colors=128, random_state=None, n_init=10):
    """Reduces the number of unique colors in a line to max_colors using k-means."""
    pixels = np.array(line, dtype=np.float32)
    h, w, _ = pixels.shape
    pixels_flat = pixels.reshape(-1, 3)
    
    unique_colors, counts = np.unique(pixels_flat, axis=0, return_counts=True)
    
    if len(unique_colors) <= max_colors:
        return line
    
    kmeans = KMeans(n_clusters=max_colors, random_state=random_state, n_init=n_init).fit(pixels_flat)
    new_colors = kmeans.cluster_centers_.astype(np.uint8)
    labels = kmeans.labels_
    
    new_line = new_colors[labels].reshape(h, w, 3)
    return new_line

def print_progress_bar(iteration, total, prefix='Progress:', suffix='Complete', decimals=1, length=50, fill='█'):
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filled_length = int(length * iteration // total)
    bar = fill * filled_length + '-' * (length - filled_length)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end='')
    if iteration == total:
        print()

def resize_to_360x288(image):
    """Resize image to 360x288 with Lanczos filter, preserving aspect ratio, centered with black padding."""
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

def convert_image(input_path, output_path, max_colors=128, use_dither=False, use_interleave=False, random_state=None, n_init=10):
    """Converts image to RGB555 format with dithering and exports to .pix and .pal."""
    try:
        img = Image.open(input_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        img = resize_to_360x288(img)
    except Exception as e:
        print(f"Error loading or resizing image: {e}")
        return
    
    pixels = img if not use_dither else floyd_steinberg_dithering(img)
    
    height, width, _ = np.array(pixels).shape
    rgb555_pixels = np.zeros((height, width, 3), dtype=np.uint8)
    for y in range(height):
        for x in range(width):
            rgb555_pixels[y, x] = rgb_to_rgb555(np.array(pixels)[y, x])
    
    max_colors_used = 0
    for y in range(height):
        line = rgb555_pixels[y:y+1, :, :]
        reduced_line = reduce_line_colors(line, max_colors, random_state, n_init)
        rgb555_pixels[y:y+1, :, :] = reduced_line
        unique_colors = len(np.unique(reduced_line.reshape(-1, 3), axis=0))
        max_colors_used = max(max_colors_used, unique_colors)
        print_progress_bar(y + 1, height)
    
    final_pixels = np.zeros((height, width, 3), dtype=np.uint8)
    for y in range(height):
        for x in range(width):
            final_pixels[y, x] = rgb555_to_rgb(rgb555_pixels[y, x])
    
    try:
        result_img = Image.fromarray(final_pixels)
        result_img = result_img.convert('RGB')
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

if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) > 7:
        print("Usage: python image_converter.py input.png output.png [max_colors] [-d] [-i] [-r random_state] [-n n_init]")
    else:
        input_path = sys.argv[1]
        output_path = sys.argv[2]
        max_colors = 128
        use_dither = False
        use_interleave = False
        random_state = None
        n_init = 10
        args = sys.argv[3:]
        i = 0
        while i < len(args):
            if args[i] == "-d":
                use_dither = True
            elif args[i] == "-i":
                use_interleave = True
            elif args[i] == "-r" and i + 1 < len(args) and args[i + 1].isdigit():
                random_state = int(args[i + 1])
                i += 1
            elif args[i] == "-n" and i + 1 < len(args) and args[i + 1].isdigit():
                n_init = int(args[i + 1])
                i += 1
            elif args[i].isdigit():
                try:
                    max_colors = int(args[i])
                    if max_colors <= 0:
                        raise ValueError
                except ValueError:
                    print("Error: max_colors must be a positive integer")
                    sys.exit(1)
            i += 1
        print(f"Parameters: input_path={input_path}, output_path={output_path}, max_colors={max_colors}, use_dither={use_dither}, use_interleave={use_interleave}, random_state={random_state}, n_init={n_init}")
        convert_image(input_path, output_path, max_colors, use_dither, use_interleave, random_state, n_init)