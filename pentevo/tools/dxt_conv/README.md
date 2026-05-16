# FT812 DXT converter

`ft812_dxt_convert.py` converts ordinary images into compact FT812-friendly textures.

It is meant for backgrounds, game screens, UI images and other full-screen or large bitmap assets where plain RGB565 is too large.

The converter makes a two-color-per-block image:

```text
raw = c0 + c1 + mask
```

- `c0` — first RGB565 color layer, one color per 4×4 block.
- `c1` — second RGB565 color layer, one color per 4×4 block.
- `mask` — full-size FT812 `L2` or `L4` selector mask that blends between `c0` and `c1`.

The image size must be divisible by 4. Use `-m crop` or `-m pad` if it is not.

## Install

CPU-only:

```bash
python3 -m pip install pillow
```

GPU/OpenCL mode:

```bash
python3 -m pip install pillow numpy pyopencl
```

GPU mode is enabled by default. Use `-c` if OpenCL is not installed or not needed.

## Quick start

Fast CPU test with preview:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l2 -t raw -e 2 -p -c
```

Final compressed DXP for the viewer:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l2 -t dxp -e 8 -z 9
```

Compare L2 and L4 previews:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l2,l4 -t raw -e 6 -p -c
```

Pad image size to a 4-pixel boundary:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l2 -t dxp -m pad -b '#000000'
```

## Recommended settings

| Goal | Command options |
|---|---|
| Quick test | `-f l2 -t raw -e 2 -p -c` |
| Normal background | `-f l2 -t dxp -e 6 -z 9` |
| Better gradients | `-f l4 -t dxp -e 6 -z 9` |
| Final high quality | `-f l2,l4 -t dxp -e 8 -z 9 -p` |
| Debug FT812 upload | `-f l2 -t raw -x -p -c` |

## Main options

```text
python3 ft812_dxt_convert.py <input> [options]
```

| Option | Default | Meaning |
|---|---:|---|
| `<input>` | required | Input image readable by Pillow: PNG, JPG, WEBP, etc. |
| `-o`, `--output-dir` | input dir | Output directory. Output file base is taken from input name. |
| `-f`, `--format` | `l2,l4` | Output texture format: `l2`, `l4`, `l2,l4`, or `all`. |
| `-t`, `--out` | `raw,dxt,dxp` | Output file type: `raw`, `dxt`, `dxp`, comma-list, or `all`. |
| `-e`, `--effort` | `4` | Optimization level `0..10`. Higher is slower. |
| `-c`, `--cpu` | off | Disable GPU and use CPU only. |
| `-g`, `--gpu` | on | Use OpenCL GPU path. Kept for old command lines. |
| `--gpu-batch` | `262144` | OpenCL batch size. Larger values may reduce overhead. |
| `-j`, `--jobs` | `0` | CPU workers. `0` means all CPU cores. |
| `-z`, `--zlib-level` | `9` | zlib compression level for `.dxp`, `0..9`. |
| `-m`, `--size-mode` | `error` | `error`, `crop`, or `pad` for non-4-aligned images. |
| `-b`, `--background` | `#000000` | Background for alpha compositing and padding. |
| `-p`, `--preview` | off | Write reconstructed preview PNG. |
| `-x`, `--split` | off | Write separate `c0`, `c1`, and mask RAW layers. |
| `--profile` | off | Print detailed timing/counter information. |

## Output files

For `image.png -o out`, output names are based on `out/image`.

### RAW

```text
out/image_l2.raw
out/image_l2.h
out/image_l4.raw
out/image_l4.h
```

RAW is the direct FT812 upload layout:

```text
c0_size = (width / 4) * (height / 4) * 2
c1_size = c0_size
mask_addr = raw_addr + c0_size + c1_size
```

L2 mask:

```text
mask_stride = width / 4
mask_size = mask_stride * height
```

L4 mask:

```text
mask_stride = width / 2
mask_size = mask_stride * height
```

### Split RAW

Generated with `-x`:

```text
out/image_l2_c0.raw
out/image_l2_c1.raw
out/image_l2_l2.raw
out/image_l4_c0.raw
out/image_l4_c1.raw
out/image_l4_l4.raw
```

Use split files when debugging addresses, strides, or FT812 bitmap handles.

### DXP

```text
out/image_l2.dxp
out/image_l4.dxp
```

DXP is the viewer-friendly container:

```text
0..2  "DXP"
3     type
4..5  width, little-endian uint16
6..7  height, little-endian uint16
8..   RAW payload, usually zlib-compressed
```

Type values:

| Type | Payload | Mask |
|---:|---|---|
| `0` | raw L2 | `FT_L2` |
| `1` | zlib L2 | `FT_L2` |
| `2` | zlib L4 | `FT_L4` |
| `3` | raw L4 | `FT_L4` |

The current converter normally writes compressed DXP:

- L2 uses type `1`.
- L4 uses type `2`.

### DXT block dump

```text
out/image_l2.dxt
out/image_l4.dxt
```

This is a debug/portable block dump, not the simplest format for direct viewer upload.

### Preview PNG

Generated with `-p`:

```text
out/image_l2_preview.png
out/image_l4_preview.png
```

Use previews to decide whether L2 is enough or L4 is needed.

## L2 or L4?

### L2 / `DXT1_L2_RGB565`

Use this first.

- Smaller mask.
- 4 blend levels per pixel.
- Good for pixel art, UI, noisy images, most backgrounds.
- RAW mask stride: `width / 4` bytes.

### L4 / `DXT1_L4_RGB565`

Use this when L2 shows visible banding.

- Larger mask.
- 16 blend levels per pixel.
- Better gradients and smooth lighting.
- RAW mask stride: `width / 2` bytes.

## Effort levels

| Level | Use for |
|---:|---|
| `0` | Very fast rough test. |
| `1` | Fast test with better endpoint candidates. |
| `2` | Quick preview. |
| `3` | Better independent block quality. |
| `4` | Default balanced mode. |
| `5` | Perceptual candidate mode. |
| `6` | First seam-aware mode. Good next step after `4`. |
| `7` | Stronger seam-aware mode. |
| `8` | Good final quality mode. |
| `9` | Seam-aware plus residual diffusion. Slow. |
| `10` | Maximum experimental mode. Slowest. |

Practical rule:

```text
-e 2   quick previews
-e 4   default work
-e 6   better block edges
-e 8   final assets
-e 10  experiments only
```

## FT812 display list

Upload the combined RAW data to `RAM_G`, then draw it with one mask pass and two RGB565 passes.

The code below is intentionally self-contained. It does not use `FT_DXP_INFO` or `info->mask_format`; the DXP type is checked inline.

```c
void ft_dxp_build_display_list(u32 raw_addr, u16 w, u16 h, u8 dxp_type, u16 x, u16 y)
{
  u32 color_size;
  u32 mask_addr;
  u16 mask_format;

  if ((w & 3) || (h & 3)) return;

  color_size = ((u32)w >> 2) * ((u32)h >> 2) * 2UL;
  mask_addr = raw_addr + color_size + color_size;

  if (dxp_type == 0 || dxp_type == 1)
  {
    mask_format = FT_L2;
  }
  else if (dxp_type == 2 || dxp_type == 3)
  {
    mask_format = FT_L4;
  }
  else
  {
    return;
  }

  ft_Dlstart();
  ft_ClearColorRGB(0, 0, 0);
  ft_ClearColorA(255);
  ft_Clear(1, 1, 1);

  ft_BitmapHandle(0);
  ft_SetBitmap(mask_addr, mask_format, w, h);

  ft_BitmapHandle(1);
  ft_SetBitmap(raw_addr, FT_RGB565, w >> 2, h >> 2);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, w, h);

  ft_Begin(FT_BITMAPS);

  ft_ColorA(255);
  ft_BlendFunc(FT_ONE, FT_ZERO);
  ft_Vertex2ii(x, y, 0, 0);

  ft_ColorMask(1, 1, 1, 0);
  ft_BitmapTransformA(64);
  ft_BitmapTransformE(64);

  ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_ZERO);
  ft_Vertex2ii(x, y, 1, 0);

  ft_BlendFunc(FT_DST_ALPHA, FT_ONE);
  ft_Vertex2ii(x, y, 1, 1);

  ft_ColorMask(1, 1, 1, 1);

  ft_Display();
  ft_Swap();
}
```

Call example after the RAW data is already uploaded:

```c
ft_ccmd_start(cmdl);
ft_dxp_build_display_list(FT_RAM_G, width, height, dxp_type, x, y);
ft_ccmd_write();
ft_cp_wait(1000);
ft_wait_swap(1000);
```

### Why this works

The color bitmap is only `width / 4` by `height / 4`, because each 4×4 image block has one `c0` and one `c1` color. `ft_BitmapTransformA(64)` and `ft_BitmapTransformE(64)` scale that compact bitmap back to full size.

FT812 bitmap cells select the two color layers:

- `ft_Vertex2ii(x, y, 1, 0)` draws `c0`.
- `ft_Vertex2ii(x, y, 1, 1)` draws `c1`.

The mask is drawn first. Its alpha then controls how much of `c0` and `c1` appears in the final image.

## Minimal DXP viewer flow

```text
1. Read whole .dxp file.
2. Check header: "DXP", type, width, height.
3. Reject width/height not divisible by 4.
4. Compute raw_size.
5. If type is 1 or 2, zlib-decompress payload to RAW.
6. If type is 0 or 3, use payload as RAW directly.
7. Upload RAW to FT_RAM_G.
8. Build and swap the display list.
```

## Troubleshooting

### `Pillow is required`

```bash
python3 -m pip install pillow
```

### OpenCL/GPU error

Use CPU mode:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l2 -t dxp -e 4 -c
```

### Image size is not divisible by 4

Pad or crop:

```bash
python3 ft812_dxt_convert.py image.png -o out -m pad -b '#000000'
```

### L2 preview has banding

Try L4:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l4 -t dxp -e 6 -p
```

### Blocks are visible

Try a higher effort level:

```bash
python3 ft812_dxt_convert.py image.png -o out -f l2 -t dxp -e 8 -p
```

### DXP does not display correctly

Check these first:

- RAW was uploaded to the same address used by the display list.
- `dxp_type` matches the file header.
- Width and height are divisible by 4.
- `mask_addr = raw_addr + c0_size + c1_size`.
- L2 uses `FT_L2`; L4 uses `FT_L4`.
- RGB565 bitmap size is `width / 4` by `height / 4`.
