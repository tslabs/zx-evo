
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include "miniz.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "driver/uart.h"

#include "main.h"
#include "font.h"
#include "spi_slave.h"
#include "ft8xx.h"
#include "sdmmc.h"
#include "zx_screen.h"
#include "esp_jpeg_dec.h"

const char TAG[] = "ft8xx";

extern const u8 code_866_fnt[];

void hexdump(const void *data, size_t len, uint64_t base_off);

#define CHUNK_PAYLOAD DMA_BUF_SIZE

// #define FT_DEMO_PIX_W       640
// #define FT_DEMO_PIX_H       480
#define FT_DEMO_PIX_W          320
#define FT_DEMO_PIX_H          240
#define FT_DEMO_PIX_SIZE       (FT_DEMO_PIX_W * FT_DEMO_PIX_H)
#define FT_DEMO_PIX0_ADDR      FT_RAM_G
#define FT_DEMO_PIX1_ADDR      (FT_RAM_G + FT_DEMO_PIX_SIZE)
#define FT_DEMO_PALETTE_ADDR   (FT_DEMO_PIX1_ADDR + FT_DEMO_PIX_SIZE)
#define FT_DEMO_PALETTE_SIZE   (256UL * 4UL)

#define FT_DEMO_TEXT_COLS                80
#define FT_DEMO_TEXT_ROWS                30
#define FT_TEXT_ATTR_MAX_COLS            160
#define FT_TEXT_ATTR_MAX_ROWS            64
#define FT_TEXT_ATTR_GROUP_ROWS          32

#define FT_DEMO_PAL4_PIX_ADDR            FT_RAM_G
#define FT_DEMO_PAL4_PAL_HI_ADDR         (FT_DEMO_PAL4_PIX_ADDR + ((FT_DEMO_TEXT_COLS * 4UL) * (FT_DEMO_TEXT_ROWS * 8UL)))
#define FT_DEMO_PAL4_PAL_LO_ADDR         (FT_DEMO_PAL4_PAL_HI_ADDR + 512UL)
#define FT_DEMO_PAL4_MASK_ADDR           0x002FF82C // Font area, value = 0x10

#define FT_TEXT_ATTR_GROUPS_FOR(rows)            ((((u32)(rows)) + FT_TEXT_ATTR_GROUP_ROWS - 1U) / FT_TEXT_ATTR_GROUP_ROWS)
#define FT_TEXT_ATTR_GFX_SIZE_FOR(cols, rows)    ((u32)(cols) * 256UL * FT_TEXT_ATTR_GROUPS_FOR(rows))
#define FT_TEXT_ATTR_COLOR_SIZE_FOR(rows)        ((u32)(rows) * 256UL)
#define FT_TEXT_ATTR_UPLOAD_SIZE_FOR(cols, rows) (FT_TEXT_ATTR_GFX_SIZE_FOR(cols, rows) + FT_TEXT_ATTR_COLOR_SIZE_FOR(rows))
#define FT_TEXT_ATTR_GFX_SIZE                    FT_TEXT_ATTR_GFX_SIZE_FOR(FT_DEMO_TEXT_COLS, FT_DEMO_TEXT_ROWS)
#define FT_TEXT_ATTR_COLOR_SIZE                  FT_TEXT_ATTR_COLOR_SIZE_FOR(FT_DEMO_TEXT_ROWS)
#define FT_TEXT_ATTR_UPLOAD_SIZE                 FT_TEXT_ATTR_UPLOAD_SIZE_FOR(FT_DEMO_TEXT_COLS, FT_DEMO_TEXT_ROWS)

#define FT_DEMO_PIX_GROUP_ADDR(group, col, cols) (FT_RAM_G + ((((u32)(group) * (u32)(cols)) + (u32)(col)) << 8))
#define FT_DEMO_PIX_ADDR(col)                    FT_DEMO_PIX_GROUP_ADDR(0, col, FT_DEMO_TEXT_COLS)
#define FT_DEMO_ATTR_ADDR_FOR(cols, rows)        (FT_RAM_G + FT_TEXT_ATTR_GFX_SIZE_FOR(cols, rows))
#define FT_DEMO_ATTR_ADDR                        FT_DEMO_ATTR_ADDR_FOR(FT_DEMO_TEXT_COLS, FT_DEMO_TEXT_ROWS)
#define FT_TEXT_ATTR_PAL_HI_ADDR_FOR(cols, rows) (FT_RAM_G + FT_TEXT_ATTR_UPLOAD_SIZE_FOR(cols, rows))
#define FT_TEXT_ATTR_PAL_LO_ADDR_FOR(cols, rows) (FT_TEXT_ATTR_PAL_HI_ADDR_FOR(cols, rows) + 512UL)

#define FT_DEMO_ZX_SCREEN_X              16
#define FT_DEMO_ZX_SCREEN_Y              16
#define FT_DEMO_ZX_PIX_ADDR              FT_RAM_G
#define FT_DEMO_ZX_ATTR_ADDR             (FT_DEMO_ZX_PIX_ADDR + 6144)
#define FT_DEMO_ZX_PAL_PAPER_ADDR        (FT_DEMO_ZX_PIX_ADDR + 0x2000)
#define FT_DEMO_ZX_PAL_INK_ADDR          (FT_DEMO_ZX_PAL_PAPER_ADDR + 512)

#define FT_RGB888_640_480_W              640
#define FT_RGB888_640_480_H              480
#define FT_RGB888_640_480_BPP            3UL
#define FT_RGB888_640_480_STRIDE         FT_RGB888_640_480_W
#define FT_RGB888_640_480_PLANE_SIZE     (FT_RGB888_640_480_STRIDE * FT_RGB888_640_480_H)
#define FT_RGB888_640_480_SIZE           (FT_RGB888_640_480_PLANE_SIZE * FT_RGB888_640_480_BPP)
#define FT_RGB888_640_480_ADDR           FT_RAM_G + ((1024U * 1024) - (FT_RGB888_640_480_W * FT_RGB888_640_480_H * FT_RGB888_640_480_BPP))

#define FT_JPG_MAX_PATH                  256
#define FT_JPG_SCAN_MAX_DEPTH            8
#define FT_DXP_HEADER_SIZE               8
#define FT_DXP_RAM_G_SIZE                (1024UL * 1024UL)

typedef struct
{
  char **items;
  int count;
  int cap;
} FT_JPG_LIST;

typedef struct
{
  u8 type;
  u16 w;
  u16 h;
  u16 mask_format;
  u32 raw_size;
}
FT_DXP_INFO;

enum
{
  FT_DXP_TYPE_RAW_L2 = 0,
  FT_DXP_TYPE_ZLIB_L2,
  FT_DXP_TYPE_ZLIB_L4,
  FT_DXP_TYPE_RAW_L4
};

enum
{
  FT_JPG_KEY_EXIT = 0,
  FT_JPG_KEY_NEXT,
  FT_JPG_KEY_PREV
};

// ------------- Profiler -------------

typedef struct
{
  uint64_t ts_loop_start;
  uint64_t ts_show_start;
  uint64_t ts_show_end;
  uint64_t ts_wait_swap_start;
  uint64_t ts_wait_swap_end;
  uint64_t ts_upload_start;
  uint64_t ts_upload_end;
  uint64_t ts_upload_done;
  uint64_t ts_render_start;
  uint64_t ts_render_end;
  uint64_t ts_idle_wait_start;
  uint64_t ts_idle_wait_end;
  u32 ft_frames_before_wait;
  u32 ft_frames_after_wait;
  int stop;
} PROFILER_LINE;

u32 *ft_ccmdb = nullptr;
u16 ft_ccmdp = 0;
EXT_RAM_BSS_ATTR u8 cmdl[8192];
u8 ft_spi_width = 2;
u32 ft_spi_freq_hz = 20000000UL;
int ft_current_mode = -1;

const FT_MODE ft_modes[] =                         //  |  # | visible  | Fpll MHz | Fpix MHz | clocks/line | lines/frame | line kHz | frame Hz |
{                                                  //  | -- | -------- | -------- | -------- | ----------- | ----------- | -------- | -------- |
  {6,  2,  16,  96,  48,  640, 11, 2, 31,  480},   //  |  0 | 640x480  |       48 |       24 |         800 |         524 |   30.000 |   57.252 |
  {8,  2,  24,  40, 128,  640,  9, 3, 28,  480},   //  |  1 | 640x480  |       64 |       32 |         832 |         520 |   38.462 |   73.964 |
  {8,  2,  16,  96,  48,  640, 11, 2, 31,  480},   //  |  2 | 640x480  |       64 |       32 |         800 |         524 |   40.000 |   76.336 |
  {5,  1,  40, 128,  88,  800,  1, 4, 23,  600},   //  |  3 | 800x600  |       40 |       40 |        1056 |         628 |   37.879 |   60.317 |
  {10, 2,  40, 128,  88,  800,  1, 4, 23,  600},   //  |  4 | 800x600  |       80 |       40 |        1056 |         628 |   37.879 |   60.317 |
  {6,  1,  56, 120,  64,  800, 37, 6, 23,  600},   //  |  5 | 800x600  |       48 |       48 |        1040 |         666 |   46.154 |   69.300 |
  {7,  1,  32,  64, 152,  800,  1, 3, 27,  600},   //  |  6 | 800x600  |       56 |       56 |        1048 |         631 |   53.435 |   84.683 |
  {8,  1,  24, 136, 160, 1024,  3, 6, 29,  768},   //  |  7 | 1024x768 |       64 |       64 |        1344 |         806 |   47.619 |   59.081 |
  {9,  1,  24, 136, 144, 1024,  3, 6, 29,  768},   //  |  8 | 1024x768 |       72 |       72 |        1328 |         806 |   54.217 |   67.267 |
  {10, 1,  16,  96, 176, 1024,  1, 3, 28,  768},   //  |  9 | 1024x768 |       80 |       80 |        1312 |         800 |   60.976 |   76.220 |
  {7,  1,  24,  56, 124,  640,  1, 3, 38, 1024},   //  | 10 | 640x1024 |       56 |       56 |         844 |        1066 |   66.351 |   62.243 |
  {9,  1, 110,  40, 220, 1280,  5, 5, 20,  720},   //  | 11 | 1280x720 |       72 |       72 |        1650 |         750 |   43.636 |   58.182 |
  {9,  1,  93,  40, 187, 1280,  5, 5, 20,  720},   //  | 12 | 1280x720 |       72 |       72 |        1600 |         750 |   45.000 |   60.000 |
};

const u16 sintab[257] =
{
  0, 402, 804, 1206, 1608, 2010, 2412, 2813, 3215, 3617, 4018, 4419, 4821, 5221, 5622, 6023,
  6423, 6823, 7223, 7622, 8022, 8421, 8819, 9218, 9615, 10013, 10410, 10807, 11203, 11599, 11995, 12390,
  12785, 13179, 13573, 13966, 14358, 14750, 15142, 15533, 15923, 16313, 16702, 17091, 17479, 17866, 18252, 18638,
  19023, 19408, 19791, 20174, 20557, 20938, 21319, 21699, 22078, 22456, 22833, 23210, 23585, 23960, 24334, 24707,
  25079, 25450, 25820, 26189, 26557, 26924, 27290, 27655, 28019, 28382, 28744, 29105, 29465, 29823, 30181, 30537,
  30892, 31247, 31599, 31951, 32302, 32651, 32999, 33346, 33691, 34035, 34378, 34720, 35061, 35400, 35737, 36074,
  36409, 36742, 37075, 37406, 37735, 38063, 38390, 38715, 39039, 39361, 39682, 40001, 40319, 40635, 40950, 41263,
  41574, 41885, 42193, 42500, 42805, 43109, 43411, 43711, 44010, 44307, 44603, 44896, 45189, 45479, 45768, 46055,
  46340, 46623, 46905, 47185, 47463, 47739, 48014, 48287, 48558, 48827, 49094, 49360, 49623, 49885, 50145, 50403,
  50659, 50913, 51165, 51415, 51664, 51910, 52155, 52397, 52638, 52876, 53113, 53347, 53580, 53810, 54039, 54265,
  54490, 54712, 54933, 55151, 55367, 55581, 55793, 56003, 56211, 56416, 56620, 56821, 57021, 57218, 57413, 57606,
  57796, 57985, 58171, 58355, 58537, 58717, 58894, 59069, 59242, 59413, 59582, 59748, 59912, 60074, 60234, 60391,
  60546, 60699, 60849, 60997, 61143, 61287, 61428, 61567, 61704, 61838, 61970, 62100, 62227, 62352, 62474, 62595,
  62713, 62828, 62941, 63052, 63161, 63267, 63370, 63472, 63570, 63667, 63761, 63853, 63942, 64029, 64114, 64196,
  64275, 64353, 64427, 64500, 64570, 64637, 64702, 64765, 64825, 64883, 64938, 64991, 65042, 65090, 65135, 65178,
  65219, 65257, 65293, 65326, 65357, 65385, 65411, 65435, 65456, 65474, 65490, 65504, 65515, 65523, 65530, 65533,
  65535
};

// ---------- Profiler -----------

QueueHandle_t profiler_queue = nullptr;
TaskHandle_t profiler_task_handle = nullptr;
TaskHandle_t profiler_done_task = nullptr;

uint64_t profiler_now()
{
  return (uint64_t)esp_timer_get_time();
}

void profiler_task(void *arg)
{
  PROFILER_LINE line;
  uint64_t prev_loop_start = 0;
  uint64_t loop_dt;
  uint64_t dt_upload;
  uint64_t dt_render;

  (void)arg;

  while (1)
  {
    if (xQueueReceive(profiler_queue, &line, portMAX_DELAY) != pdTRUE)
      continue;

    if (line.stop)
      break;

    loop_dt = prev_loop_start ? (line.ts_loop_start - prev_loop_start) : 0;
    prev_loop_start = line.ts_loop_start;

    dt_upload = (line.ts_upload_start && line.ts_upload_done) ? (line.ts_upload_done - line.ts_upload_start) : 0;
    dt_render = (line.ts_render_start && line.ts_render_end) ? (line.ts_render_end - line.ts_render_start) : 0;

    printf("us, frame=%llu ", (unsigned long long)loop_dt);
    printf("render=%llu ", (unsigned long long)dt_render);
    printf("xfer=%llu", (unsigned long long)dt_upload);
    printf("\r\n");
  }

  if (profiler_done_task)
    xTaskNotifyGive(profiler_done_task);

  vTaskDelete(nullptr);
}

esp_err_t profiler_start()
{
  if (profiler_queue)
    return ESP_OK;

  profiler_queue = xQueueCreate(64, sizeof(PROFILER_LINE));
  if (!profiler_queue)
    return ESP_ERR_NO_MEM;

  profiler_done_task = xTaskGetCurrentTaskHandle();

  if (xTaskCreatePinnedToCore(profiler_task, "profiler_log", 4096, nullptr, tskIDLE_PRIORITY + 1, &profiler_task_handle, 1) != pdPASS)
  {
    vQueueDelete(profiler_queue);
    profiler_queue = nullptr;
    profiler_done_task = nullptr;
    profiler_task_handle = nullptr;
    return ESP_FAIL;
  }

  return ESP_OK;
}

void profiler_push(const PROFILER_LINE *line)
{
  if (!profiler_queue || !line)
    return;

  xQueueSend(profiler_queue, line, 0);
}

void profiler_stop()
{
  PROFILER_LINE line = {};
  QueueHandle_t queue = profiler_queue;

  if (!queue)
    return;

  line.stop = 1;
  xQueueSend(queue, &line, pdMS_TO_TICKS(1000));

  if (!ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) && profiler_task_handle)
    vTaskDelete(profiler_task_handle);

  profiler_queue = nullptr;
  profiler_task_handle = nullptr;
  profiler_done_task = nullptr;

  vQueueDelete(queue);
}

// -------- Aux math ---------

i16 rsin(i16 r, u16 th)
{
  i16 th4;
  u16 s;
  i16 p;

  th >>= 6;
  th4 = (i16)(th & 511);

  if (th4 & 256)
    th4 = 512 - th4;

  s = sintab[(u16)th4];
  p = (i16)(((u32)s * (u32)r) >> 16);

  if (th & 512)
    p = -p;

  return p;
}

i16 rcos(i16 r, u16 th)
{
  return rsin(r, (u16)(th + 0x4000));
}

u32 ft_argb32(u8 a, u8 r, u8 g, u8 b)
{
  return ((u32)a << 24) | ((u32)r << 16) | ((u32)g << 8) | (u32)b;
}

// -------- Display list ---------

void ft_AlphaFunc(u8 func, u8 ref)
{
  ft_ccmd((9UL << 24) | ((func & 7L) << 8) | ((ref & 255L) << 0));
}

void ft_Begin(u8 prim)
{
  ft_ccmd((31UL << 24) | prim);
}

void ft_BitmapHandle(u8 handle)
{
  ft_ccmd((5UL << 24) | handle);
}

void ft_BitmapLayout(u8 format, u16 linestride, u16 height)
{
  ft_ccmd((0x28UL << 24) | ((linestride >> 8) & 12L) | ((height >> 9) & 3L));
  ft_ccmd((7UL << 24) | ((format & 31L) << 19) | ((linestride & 1023L) << 9) | ((height & 511L) << 0));
}

void ft_BitmapSize(u8 filter, u8 wrapx, u8 wrapy, u16 width, u16 height)
{
  u8 fxy = (filter << 2) | (wrapx << 1) | (wrapy);
  ft_ccmd((0x29UL << 24) | ((width >> 7) & 12L) | ((height>> 9) & 3L));
  ft_ccmd((8UL << 24) | ((u32)fxy << 18) | ((width & 511L) << 9) | ((height & 511L) << 0));
}

void ft_BitmapSource(u32 addr)
{
  ft_ccmd((1UL << 24) | ((addr & 0x3FFFFFL) << 0));
}

void ft_BitmapTransformA(i32 a)
{
  ft_ccmd((21UL << 24) | ((a & 131071L) << 0));
}

void ft_BitmapTransformB(i32 b)
{
  ft_ccmd((22UL << 24) | ((b & 131071L) << 0));
}

void ft_BitmapTransformC(i32 c)
{
  ft_ccmd((23UL << 24) | ((c & 16777215L) << 0));
}

void ft_BitmapTransformD(i32 d)
{
  ft_ccmd((24UL << 24) | ((d & 131071L) << 0));
}

void ft_BitmapTransformE(i32 e)
{
  ft_ccmd((25UL << 24) | ((e & 131071L) << 0));
}

void ft_BitmapTransformF(i32 f)
{
  ft_ccmd((26UL << 24) | ((f & 16777215L) << 0));
}

void ft_BlendFunc(u8 src, u8 dst)
{
  ft_ccmd((11UL << 24) | ((src & 7L) << 3) | ((dst & 7L) << 0));
}

void ft_Call(u16 dest)
{
  ft_ccmd((29UL << 24) | ((dest & 2047L) << 0));
}

void ft_Cell(u8 cell)
{
  ft_ccmd((6UL << 24) | ((cell & 127L) << 0));
}

void ft_ClearColorA(u8 alpha)
{
  ft_ccmd((15UL << 24) | ((alpha & 255L) << 0));
}

void ft_ClearColorRGB(u8 red, u8 green, u8 blue)
{
  ft_ccmd((2UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

void ft_ClearColorRGB32(u32 rgb)
{
  ft_ccmd((2UL << 24) | (rgb & 0xffffffL));
}

void ft_Clear(u8 c, u8 s, u8 t)
{
  u8 m = (c << 2) | (s << 1) | t;
  ft_ccmd((38UL << 24) | m);
}

void ft_ClearAll()
{
  ft_ccmd((38UL << 24) | 7);
}

void ft_ClearStencil(u8 s)
{
  ft_ccmd((17UL << 24) | ((s & 255L) << 0));
}

void ft_ClearTag(u8 s)
{
  ft_ccmd((18UL << 24) | ((s & 255L) << 0));
}

void ft_ColorA(u8 alpha)
{
  ft_ccmd((16UL << 24) | ((alpha & 255L) << 0));
}

void ft_ColorMask(u8 r, u8 g, u8 b, u8 a)
{
  ft_ccmd((32UL << 24) | ((r & 1L) << 3) | ((g & 1L) << 2) | ((b & 1L) << 1) | ((a & 1L) << 0));
}

void ft_ColorRGB(u8 red, u8 green, u8 blue)
{
  ft_ccmd((4UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}

void ft_ColorRGB32(u32 rgb)
{
  ft_ccmd((4UL << 24) | (rgb & 0xffffffL));
}

void ft_Display(void)
{
  ft_ccmd(0UL << 24);
}

void ft_End(void)
{
  ft_ccmd(33UL << 24);
}

void ft_Jump(u16 dest)
{
  ft_ccmd((30UL << 24) | ((dest & 2047L) << 0));
}

void ft_LineWidth(u16 width)
{
  ft_ccmd((14UL << 24) | ((width & 4095L) << 0));
}

void ft_Macro(u8 m)
{
  ft_ccmd((37UL << 24) | ((m & 1L) << 0));
}

void ft_PaletteSource(u32 addr)
{
  ft_ccmd((0x2AUL << 24) | ((addr & 0x3FFFFFL) << 0));
}

void ft_PointSize(u16 size)
{
  ft_ccmd((13UL << 24) | ((size & 8191L) << 0));
}

void ft_RestoreContext(void)
{
  ft_ccmd(35UL << 24);
}

void ft_Return(void)
{
  ft_ccmd(36UL << 24);
}

void ft_SaveContext(void)
{
  ft_ccmd(34UL << 24);
}

void ft_ScissorSize(u16 width, u16 height)
{
  ft_ccmd((28UL << 24) | ((width & 4095L) << 12) | ((height & 4095L) << 0));
}

void ft_ScissorXY(u16 x, u16 y)
{
  ft_ccmd((27UL << 24) | ((x & 511L) << 9) | ((y & 511L) << 0));
}

void ft_StencilFunc(u8 func, u8 ref, u8 mask)
{
  ft_ccmd((10UL << 24) | ((func & 7L) << 16) | ((ref & 255L) << 8) | ((mask & 255L) << 0));
}

void ft_StencilMask(u8 mask)
{
  ft_ccmd((19UL << 24) | ((mask & 255L) << 0));
}

void ft_StencilOp(u8 sfail, u8 spass)
{
  ft_ccmd((12UL << 24) | ((sfail & 7L) << 3) | ((spass & 7L) << 0));
}

void ft_TagMask(u8 mask)
{
  ft_ccmd((20UL << 24) | ((mask & 1L) << 0));
}

void ft_Tag(u8 s)
{
  ft_ccmd((3UL << 24) | s);
}

void ft_Vertex2f(i16 x, i16 y)
{
  ft_ccmd((1UL << 30) | ((x & 32767L) << 15) | ((y & 32767L) << 0));
}

void ft_Vertex2ii(u16 x, u16 y, u8 handle, u8 cell)
{
  ft_ccmd((2UL << 30) | ((x & 511L) << 21) | ((y & 511L) << 12) | ((handle & 31L) << 7) | ((cell & 127L) << 0));
}

void ft_VertexFormat(u8 f)
{
  ft_ccmd((0x27UL << 24) | (f & 7L));
}

void ft_VertexTranslateX(i32 v)
{
  ft_ccmd((0x2BUL << 24) | ((u32)v & 0x1FFFF));
}

void ft_VertexTranslateY(i32 v)
{
  ft_ccmd((0x2CUL << 24) | ((u32)v & 0x1FFFF));
}

// Co-processor
void ft_SetBitmap(u32 source, u16 fmt, u16 w, u16 h)
{
  ft_ccmd(FT_CCMD_SETBITMAP);
  ft_ccmd(source);
  ft_ccmd((((u32)w << 16) | (fmt & 0xffff)));
  ft_ccmd(h);
}

void ft_SetScratch(u32 handle)
{
  ft_ccmd(FT_CCMD_SETSCRATCH);
  ft_ccmd(handle);
}

void ft_Text(i16 x, i16 y, i16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_TEXT);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_Number(i16 x, i16 y, i16 font, u16 options, i32 n)
{
  ft_ccmd(FT_CCMD_NUMBER);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_ccmd(n);
}

void ft_LoadIdentity()
{
  ft_ccmd(FT_CCMD_LOADIDENTITY);
}

void ft_Toggle(i16 x, i16 y, i16 w, i16 font, u16 options, u16 state, const char *s)
{
  ft_ccmd(FT_CCMD_TOGGLE);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)font << 16) | (w & 0xffff)));
  ft_ccmd((((u32)state << 16) | options));
  ft_cstr(s);
}

void ft_Gauge(i16 x, i16 y, i16 r, u16 options, u16 major, u16 minor, u16 val, u16 range)
{
  ft_ccmd(FT_CCMD_GAUGE);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd((((u32)minor << 16) | (major & 0xffff)));
  ft_ccmd((((u32)range << 16) | (val & 0xffff)));
}

void ft_RegRead(u32 ptr, u32 result)
{
  ft_ccmd(FT_CCMD_REGREAD);
  ft_ccmd(ptr);
  ft_ccmd(result);
}

void ft_VideoStart()
{
  ft_ccmd(FT_CCMD_VIDEOSTART);
}

void ft_GetProps(u32 ptr, u32 w, u32 h)
{
  ft_ccmd(FT_CCMD_GETPROPS);
  ft_ccmd(ptr);
  ft_ccmd(w);
  ft_ccmd(h);
}

void ft_Memcpy(u32 dest, u32 src, u32 num)
{
  ft_ccmd(FT_CCMD_MEMCPY);
  ft_ccmd(dest);
  ft_ccmd(src);
  ft_ccmd(num);
}

void ft_Spinner(i16 x, i16 y, u16 style, u16 scale)
{
  ft_ccmd(FT_CCMD_SPINNER);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)scale << 16) | (style & 0xffff)));
}

void ft_BgColor(u32 c)
{
  ft_ccmd(FT_CCMD_BGCOLOR);
  ft_ccmd(c);
}

void ft_Swap()
{
  ft_ccmd(FT_CCMD_SWAP);
}

void ft_Inflate(u32 ptr)
{
  ft_ccmd(FT_CCMD_INFLATE);
  ft_ccmd(ptr);
}

void ft_Translate(i32 tx, i32 ty)
{
  ft_ccmd(FT_CCMD_TRANSLATE);
  ft_ccmd(tx);
  ft_ccmd(ty);
}

void ft_Stop()
{
  ft_ccmd(FT_CCMD_STOP);
}

void ft_SetBase(u32 base)
{
  ft_ccmd(FT_CCMD_SETBASE);
  ft_ccmd(base);
}

void ft_Slider(i16 x, i16 y, i16 w, i16 h, u16 options, u16 val, u16 range)
{
  ft_ccmd(FT_CCMD_SLIDER);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd(range);
}

void ft_VideoFrame(u32 dst, u32 ptr)
{
  ft_ccmd(FT_CCMD_VIDEOFRAME);
  ft_ccmd(dst);
  ft_ccmd(ptr);
}

void ft_TouchTransform(i32 x0, i32 y0, i32 x1, i32 y1, i32 x2, i32 y2, i32 tx0, i32 ty0, i32 tx1, i32 ty1, i32 tx2, i32 ty2, u16 result)
{
  ft_ccmd(FT_CCMD_TOUCH_TRANSFORM);
  ft_ccmd(x0);
  ft_ccmd(y0);
  ft_ccmd(x1);
  ft_ccmd(y1);
  ft_ccmd(x2);
  ft_ccmd(y2);
  ft_ccmd(tx0);
  ft_ccmd(ty0);
  ft_ccmd(tx1);
  ft_ccmd(ty1);
  ft_ccmd(tx2);
  ft_ccmd(ty2);
  ft_ccmd(result);
}

void ft_Interrupt(u32 ms)
{
  ft_ccmd(FT_CCMD_INTERRUPT);
  ft_ccmd(ms);
}

void ft_FgColor(u32 c)
{
  ft_ccmd(FT_CCMD_FGCOLOR);
  ft_ccmd(c);
}

void ft_Rotate(i32 a)
{
  ft_ccmd(FT_CCMD_ROTATE);
  ft_ccmd(a);
}

void ft_Button(i16 x, i16 y, i16 w, i16 h, i16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_BUTTON);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_MemWrite(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_MEMWRITE);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_Scrollbar(i16 x, i16 y, i16 w, i16 h, u16 options, u16 val, u16 size, u16 range)
{
  ft_ccmd(FT_CCMD_SCROLLBAR);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd((((u32)range << 16) | (size & 0xffff)));
}

void ft_GetMatrix(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f)
{
  ft_ccmd(FT_CCMD_GETMATRIX);
  ft_ccmd(a);
  ft_ccmd(b);
  ft_ccmd(c);
  ft_ccmd(d);
  ft_ccmd(e);
  ft_ccmd(f);
}

void ft_Sketch(i16 x, i16 y, u16 w, u16 h, u32 ptr, u16 format)
{
  ft_ccmd(FT_CCMD_SKETCH);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd(ptr);
  ft_ccmd(format);
}

void ft_RomFont(u32 font, u32 romslot)
{
  ft_ccmd(FT_CCMD_ROMFONT);
  ft_ccmd(font);
  ft_ccmd(romslot);
}

void ft_PlayVideo(u32 options)
{
  ft_ccmd(FT_CCMD_PLAYVIDEO);
  ft_ccmd(options);
}

void ft_MemSet(u32 ptr, u32 value, u32 num)
{
  ft_ccmd(FT_CCMD_MEMSET);
  ft_ccmd(ptr);
  ft_ccmd(value);
  ft_ccmd(num);
}

void ft_GradColor(u32 c)
{
  ft_ccmd(FT_CCMD_GRADCOLOR);
  ft_ccmd(c);
}

void ft_Sync()
{
  ft_ccmd(FT_CCMD_SYNC);
}

void ft_BitmapTransform(i32 x0, i32 y0, i32 x1, i32 y1, i32 x2, i32 y2, i32 tx0, i32 ty0, i32 tx1, i32 ty1, i32 tx2, i32 ty2, u16 result)
{
  ft_ccmd(FT_CCMD_BITMAP_TRANSFORM);
  ft_ccmd(x0);
  ft_ccmd(y0);
  ft_ccmd(x1);
  ft_ccmd(y1);
  ft_ccmd(x2);
  ft_ccmd(y2);
  ft_ccmd(tx0);
  ft_ccmd(ty0);
  ft_ccmd(tx1);
  ft_ccmd(ty1);
  ft_ccmd(tx2);
  ft_ccmd(ty2);
  ft_ccmd(result);
}

void ft_Calibrate(u32 result)
{
  ft_ccmd(FT_CCMD_CALIBRATE);
  ft_ccmd(result);
}

void ft_SetFont(u32 font, u32 ptr)
{
  ft_ccmd(FT_CCMD_SETFONT);
  ft_ccmd(font);
  ft_ccmd(ptr);
}

void ft_Logo()
{
  ft_ccmd(FT_CCMD_LOGO);
}

void ft_Append(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_APPEND);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_MemZero(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_MEMZERO);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_Scale(i32 sx, i32 sy)
{
  ft_ccmd(FT_CCMD_SCALE);
  ft_ccmd(sx);
  ft_ccmd(sy);
}

void ft_Clock(i16 x, i16 y, i16 r, u16 options, u16 h, u16 m, u16 s, u16 ms)
{
  ft_ccmd(FT_CCMD_CLOCK);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd((((u32)m << 16) | (h & 0xffff)));
  ft_ccmd((((u32)ms << 16) | (s & 0xffff)));
}

void ft_Gradient(i16 x0, i16 y0, u32 rgb0, i16 x1, i16 y1, u32 rgb1)
{
  ft_ccmd(FT_CCMD_GRADIENT);
  ft_ccmd((((u32)y0 << 16) | (x0 & 0xffff)));
  ft_ccmd(rgb0);
  ft_ccmd((((u32)y1 << 16) | (x1 & 0xffff)));
  ft_ccmd(rgb1);
}

void ft_SetMatrix()
{
  ft_ccmd(FT_CCMD_SETMATRIX);
}

void ft_Track(i16 x, i16 y, i16 w, i16 h, i16 tag)
{
  ft_ccmd(FT_CCMD_TRACK);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd(tag);
}

void ft_Int_RAMShared(u32 ptr)
{
  ft_ccmd(FT_CCMD_INT_RAMSHARED);
  ft_ccmd(ptr);
}

void ft_Int_SWLoadImage(u32 ptr, u32 options)
{
  ft_ccmd(FT_CCMD_INT_SWLOADIMAGE);
  ft_ccmd(ptr);
  ft_ccmd(options);
}

void ft_GetPtr(u32 result)
{
  ft_ccmd(FT_CCMD_GETPTR);
  ft_ccmd(result);
}

void ft_Progress(i16 x, i16 y, i16 w, i16 h, u16 options, u16 val, u16 range)
{
  ft_ccmd(FT_CCMD_PROGRESS);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)val << 16) | (options & 0xffff)));
  ft_ccmd(range);
}

void ft_ColdStart()
{
  ft_ccmd(FT_CCMD_COLDSTART);
}

void ft_MediaFifo(u32 ptr, u32 size)
{
  ft_ccmd(FT_CCMD_MEDIAFIFO);
  ft_ccmd(ptr);
  ft_ccmd(size);
}

void ft_Keys(i16 x, i16 y, i16 w, i16 h, i16 font, u16 options, const char *s)
{
  ft_ccmd(FT_CCMD_KEYS);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
  ft_ccmd((((u32)options << 16) | (font & 0xffff)));
  ft_cstr(s);
}

void ft_Dial(i16 x, i16 y, i16 r, u16 options, u16 val)
{
  ft_ccmd(FT_CCMD_DIAL);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)options << 16) | (r & 0xffff)));
  ft_ccmd(val);
}

void ft_Snapshot2(u32 fmt, u32 ptr, i16 x, i16 y, i16 w, i16 h)
{
  ft_ccmd(FT_CCMD_SNAPSHOT2);
  ft_ccmd(fmt);
  ft_ccmd(ptr);
  ft_ccmd((((u32)y << 16) | (x & 0xffff)));
  ft_ccmd((((u32)h << 16) | (w & 0xffff)));
}

void ft_LoadImage(u32 ptr, u32 options)
{
  ft_ccmd(FT_CCMD_LOADIMAGE);
  ft_ccmd(ptr);
  ft_ccmd(options);
}

void ft_SetFont2(u32 font, u32 ptr, u32 firstchar)
{
  ft_ccmd(FT_CCMD_SETFONT2);
  ft_ccmd(font);
  ft_ccmd(ptr);
  ft_ccmd(firstchar);
}

void ft_SetRotate(u32 r)
{
  ft_ccmd(FT_CCMD_SETROTATE);
  ft_ccmd(r);
}

void ft_Dlstart()
{
  ft_ccmd(FT_CCMD_DLSTART);
}

void ft_Snapshot(u32 ptr)
{
  ft_ccmd(FT_CCMD_SNAPSHOT);
  ft_ccmd(ptr);
}

void ft_ScreenSaver()
{
  ft_ccmd(FT_CCMD_SCREENSAVER);
}

void ft_MemCrc(u32 ptr, u32 num, u32 result)
{
  ft_ccmd(FT_CCMD_MEMCRC);
  ft_ccmd(ptr);
  ft_ccmd(num);
  ft_ccmd(result);
}

void ft_FlashAttach()
{
  ft_ccmd(FT_CCMD_FLASHATTACH);
}

void ft_FlashDetach()
{
  ft_ccmd(FT_CCMD_FLASHDETACH);
}

void ft_FlashFast(u32 rc)
{
  ft_ccmd(FT_CCMD_FLASHFAST);
  ft_ccmd(rc);
}

void ft_FlashSpiDesel()
{
  ft_ccmd(FT_CCMD_FLASHSPIDESEL);
}

void ft_FlashTx(u32 num)
{
  ft_ccmd(FT_CCMD_FLASHTX);
  ft_ccmd(num);
}

void ft_FlashRx(u32 ptr, u32 num)
{
  ft_ccmd(FT_CCMD_FLASHRX);
  ft_ccmd(ptr);
  ft_ccmd(num);
}

void ft_FlashErase()
{
  ft_ccmd(FT_CCMD_FLASHERASE);
}

void ft_FlashUpdate(u32 dest, u32 src, u32 num)
{
  ft_ccmd(FT_CCMD_FLASHUPDATE);
  ft_ccmd(dest);
  ft_ccmd(src);
  ft_ccmd(num);
}

// ------------- System layer ---------------

void init_ft8xx()
{
}

// ------------- Hardware layer ---------------

esp_err_t ft_apply_spi_width(u8 width)
{
  switch (width)
  {
    case 1:
      ft_wreg8(FT_REG_SPI_WIDTH, FT_SPI_WIDTH_SINGLE);
      return spi_master_set_data_lines(1);

    case 2:
      ft_wreg8(FT_REG_SPI_WIDTH, FT_SPI_WIDTH_DUAL);
      return spi_master_set_data_lines(2);

    case 4:
      ft_wreg8(FT_REG_SPI_WIDTH, FT_SPI_WIDTH_QUAD | 4);
      return spi_master_set_data_lines(4);
  }

  return ESP_ERR_INVALID_ARG;
}

esp_err_t ft_switch_spi_to_1_bit()
{
  return ft_apply_spi_width(1);
}

esp_err_t ft_switch_spi_to_2_bit()
{
  return ft_apply_spi_width(2);
}

esp_err_t ft_switch_spi_to_4_bit()
{
  return ft_apply_spi_width(4);
}

esp_err_t ft_open_session()
{
  esp_err_t err;

  spi_master_set_clock_hz(ft_spi_freq_hz);

  err = spi_switch_to_master();
  if (err != ESP_OK) return err;

  err = spi_master_set_data_lines(1);
  if (err != ESP_OK)
  {
    spi_switch_to_slave();
    return err;
  }

  err = ft_apply_spi_width(ft_spi_width);
  if (err != ESP_OK)
  {
    spi_switch_to_slave();
    return err;
  }

  return ESP_OK;
}

esp_err_t ft_close_session()
{
  esp_err_t err;
  esp_err_t err2;

  err = ft_switch_spi_to_1_bit();

  err2 = spi_switch_to_slave();
  if (err == ESP_OK)
    err = err2;

  return err;
}

esp_err_t ft_xfer(const void *tx, void *rx, size_t size)
{
  return spi_master_xfer((void*)tx, rx, size);
}

inline void wr16le(u8 *p, u16 v)
{
  p[0] = (u8)(v & 0xFF);
  p[1] = (u8)((v >> 8) & 0xFF);
}

inline void wr32le(u8 *p, u32 v)
{
  p[0] = (u8)(v & 0xFF);
  p[1] = (u8)((v >> 8) & 0xFF);
  p[2] = (u8)((v >> 16) & 0xFF);
  p[3] = (u8)((v >> 24) & 0xFF);
}

esp_err_t ft_cmdp(u8 a, u8 v)
{
  u8 tx[3] = { a, v, 0 };
  return ft_xfer(tx, nullptr, sizeof(tx));
}

esp_err_t ft_cmd(u8 a)
{
  return ft_cmdp(a, 0);
}

void ft_wreg8(u32 a, u8 v)
{
  u8 tx[4];
  tx[0] = (u8)((FT_RAM_REG >> 16) | 0x80);
  tx[1] = (u8)(a >> 8);
  tx[2] = (u8)(a & 0xFF);
  tx[3] = v;
  ft_xfer(tx, nullptr, sizeof(tx));
}

void ft_wreg16(u32 a, u16 v)
{
  u8 tx[5];
  tx[0] = (u8)((FT_RAM_REG >> 16) | 0x80);
  tx[1] = (u8)(a >> 8);
  tx[2] = (u8)(a & 0xFF);
  wr16le(&tx[3], v);
  ft_xfer(tx, nullptr, sizeof(tx));
}

void ft_wreg32(u32 a, u32 v)
{
  u8 tx[7];
  tx[0] = (u8)((FT_RAM_REG >> 16) | 0x80);
  tx[1] = (u8)(a >> 8);
  tx[2] = (u8)(a & 0xFF);
  wr32le(&tx[3], v);
  ft_xfer(tx, nullptr, sizeof(tx));
}

u8 ft_rreg8(u32 a)
{
  u8 tx[5] =
  {
    (u8)(FT_RAM_REG >> 16),
    (u8)(a >> 8),
    (u8)(a & 0xFF),
    0,
    0
  };
  u8 rx[5] = {};

  ft_xfer(tx, rx, sizeof(tx));

  return rx[4];
}

u16 ft_rreg16(u32 a)
{
  u8 tx[6] =
  {
    (u8)(FT_RAM_REG >> 16),
    (u8)(a >> 8),
    (u8)(a & 0xFF),
    0,
    0,
    0
  };
  u8 rx[6] = {};

  ft_xfer(tx, rx, sizeof(tx));

  return (u16)rx[4] | ((u16)rx[5] << 8);
}

u32 ft_rreg32(u32 a)
{
  u8 tx[8] =
  {
    (u8)(FT_RAM_REG >> 16),
    (u8)(a >> 8),
    (u8)(a & 0xFF),
    0,
    0,
    0,
    0,
    0
  };
  u8 rx[8] = {};

  ft_xfer(tx, rx, sizeof(tx));

  return (u32)rx[4] | ((u32)rx[5] << 8) | ((u32)rx[6] << 16) | ((u32)rx[7] << 24);
}

esp_err_t ft_write(const void *addr, u32 ft_addr, u32 size)
{
  if (!addr && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  const u8 *src = (const u8*)addr;
  esp_err_t err = ESP_OK;
  u32 a = ft_addr;
  u32 left = size;

  while (left)
  {
    u32 n = left > CHUNK_PAYLOAD ? CHUNK_PAYLOAD : left;

    err = spi_master_write_buf((u8)(((a >> 16) & 0x3F) | 0x80), (u16)a, src, n);
    if (err != ESP_OK)
      break;

    src += n;
    a += n;
    left -= n;
  }

  return err;
}

esp_err_t ft_read(void *addr, u32 ft_addr, u32 size)
{
  if (!addr && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  u8 *dst = (u8*)addr;
  esp_err_t err = ESP_OK;
  u32 a = ft_addr;
  u32 left = size;

  while (left)
  {
    u32 n = left > CHUNK_PAYLOAD ? CHUNK_PAYLOAD : left;

    err = spi_master_read_buf((u8)((a >> 16) & 0x3F), (u16)a, dst, n);
    if (err != ESP_OK)
      break;

    dst += n;
    a += n;
    left -= n;
  }

  return err;
}

esp_err_t ft_write_dl(const void *addr, u32 size_dwords)
{
  return ft_write(addr, FT_RAM_DL, size_dwords << 2);
}

void ft_ccmd_start(void *addr)
{
  ft_ccmdb = (u32*)addr;
  ft_ccmdp = 0;
}

void ft_ccmd(u32 a)
{
  ft_ccmdb[ft_ccmdp++] = a;
}

void ft_cstr(const char *s)
{
  u8 *p = (u8*)&ft_ccmdb[ft_ccmdp];
  u16 c = 0;

  while ((p[c++] = (u8)*s++) != 0)
  {}

  ft_ccmdp += (c + 3) >> 2;
}

esp_err_t ft_ccmd_write()
{
  return ft_write((const void*)ft_ccmdb, FT_REG_CMDB_WRITE, (u32)ft_ccmdp << 2);
}

esp_err_t ft_cp_wait(uint32_t timeout_ms)
{
  int64_t t0 = esp_timer_get_time();
  while (1)
  {
    u16 space = ft_rreg16(FT_REG_CMDB_SPACE);

    if (space == 0x0FFC)
      return ESP_OK;

    if (((esp_timer_get_time() - t0) / 1000) >= (int64_t)timeout_ms)
      return ESP_ERR_TIMEOUT;

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

esp_err_t ft_cp_reset()
{
  ft_wreg8(FT_REG_CPURESET, 1);
  ft_wreg32(FT_REG_CMD_READ, 0);
  ft_wreg32(FT_REG_CMD_WRITE, 0);
  ft_wreg8(FT_REG_CPURESET, 0);

  return ESP_OK;
}

void ft_swap()
{
  return ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
}

esp_err_t ft_wait_swap(uint32_t timeout_ms)
{
  int64_t t0 = esp_timer_get_time();
  while (1)
  {
    u8 dlswap = ft_rreg8(FT_REG_DLSWAP);

    if (dlswap == FT_DLSWAP_DONE)
      return ESP_OK;

    if (((esp_timer_get_time() - t0) / 1000) >= (int64_t)timeout_ms)
      return ESP_ERR_TIMEOUT;

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

esp_err_t ft_set_mode(u8 m)
{
  static int last_f_mul = -1;

  if (m > FT_MODE_MAX)
    return ESP_ERR_INVALID_ARG;

  const FT_MODE *mode = &ft_modes[m];
  esp_err_t err;


  err = ft_switch_spi_to_1_bit();
  if (err != ESP_OK) return err;

  if (last_f_mul != mode->f_mul)
  {
    ft_current_mode = -1;
    last_f_mul = -1;

    ft_cmd(FT_CMD_PWRDOWN);
    ft_cmd(FT_CMD_ACTIVE);
    ft_cmd(FT_CMD_SLEEP);
    ft_cmd(FT_CMD_CLKEXT);
    ft_cmdp(FT_CMD_CLKSEL, mode->f_mul | 0xC0);
    ft_cmd(FT_CMD_ACTIVE);

    int64_t t0 = esp_timer_get_time();
    while (1)
    {
      u8 id = ft_rreg8(FT_REG_ID);
      if (id == FT_ID) break;

      if ((esp_timer_get_time() - t0) > 100000)
      {
        err = ESP_ERR_TIMEOUT;
        goto fail;
      }

      vTaskDelay(pdMS_TO_TICKS(1));
    }

    last_f_mul = mode->f_mul;
  }

  ft_wreg16(FT_REG_HSYNC0, mode->h_fporch);
  ft_wreg16(FT_REG_HSYNC1, mode->h_fporch + mode->h_sync);
  ft_wreg16(FT_REG_HOFFSET, mode->h_fporch + mode->h_sync + mode->h_bporch);
  ft_wreg16(FT_REG_HCYCLE, mode->h_fporch + mode->h_sync + mode->h_bporch + mode->h_visible);
  ft_wreg16(FT_REG_HSIZE, mode->h_visible);
  ft_wreg16(FT_REG_VSYNC0, mode->v_fporch - 1);
  ft_wreg16(FT_REG_VSYNC1, mode->v_fporch + mode->v_sync - 1);
  ft_wreg16(FT_REG_VOFFSET, mode->v_fporch + mode->v_sync + mode->v_bporch - 1);
  ft_wreg16(FT_REG_VCYCLE, mode->v_fporch + mode->v_sync + mode->v_bporch + mode->v_visible);
  ft_wreg16(FT_REG_VSIZE, mode->v_visible);
  ft_wreg8(FT_REG_PCLK_POL, 0);
  ft_wreg8(FT_REG_CSPREAD, 0);
  ft_wreg8(FT_REG_PCLK, mode->f_div);
  ft_wreg8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wreg8(FT_REG_INT_EN, 1);

  vTaskDelay(pdMS_TO_TICKS(30));
  ft_wreg32(FT_REG_FREQUENCY, mode->f_mul * 8000000UL);

  err = ft_apply_spi_width(ft_spi_width);
  if (err != ESP_OK) return err;

  ft_current_mode = m;
  return ESP_OK;

fail:
  ft_apply_spi_width(ft_spi_width);
  return err;
}

esp_err_t ft_read_detect(u8 &chip_id, u8 chip_type[4], char datestamp[17])
{
  chip_id = ft_rreg8(FT_REG_ID);
  ft_read(chip_type, FT_ROM_CHIPID, 4);
  ft_read(datestamp, FT_REG_DATESTAMP, 16);
  datestamp[16] = 0;

  return ESP_OK;
}

esp_err_t ft_reset_chip()
{
  ft_current_mode = -1;

  esp_err_t err = ft_switch_spi_to_1_bit();
    if (err != ESP_OK) return err;

  ft_cmd(FT_CMD_PWRDOWN);
  ft_cmd(FT_CMD_ACTIVE);
  ft_cmd(FT_CMD_SLEEP);
  ft_cmd(FT_CMD_CLKEXT);
  ft_cmd(FT_CMD_ACTIVE);
  ft_cmd(FT_CMD_RST_PULSE);

  u8 chip_id;
  int64_t t0 = esp_timer_get_time();

  while ((esp_timer_get_time() - t0) < 100000)
  {
    chip_id = ft_rreg8(FT_REG_ID);
    if (chip_id == FT_ID) break;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  vTaskDelay(pdMS_TO_TICKS(30));
  ft_wreg32(FT_REG_FREQUENCY, 40000000UL);

  err = ft_apply_spi_width(ft_spi_width);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

// ------------- Console ---------------

u32 ft_parse_num_arg(const char *s, const char *name, int *ok);

void ft_print_string(const char *s, u16 x, u16 y)
{
  int i = 0;
  char c;

  while ((c = s[i++]) != 0)
  {
    ft_Cell((u8)c);
    ft_Vertex2f((i16)x, (i16)y);
    x += 9;
  }
}

void ft_print_spi_freq(const char *label, u32 hz)
{
  printf("%s: %lu.%03lu MHz (%lu Hz)\r\n",
         label,
         (unsigned long)(hz / 1000000UL),
         (unsigned long)((hz % 1000000UL) / 1000UL),
         (unsigned long)hz);
}

void ft_print_current_mode()
{
  if (ft_current_mode < 0)
  {
    printf("FT mode: none\r\n");
    return;
  }

  const FT_MODE *mode = &ft_modes[ft_current_mode];
  printf("FT mode: %d, %ux%u\r\n",
         ft_current_mode,
         (unsigned int)mode->h_visible,
         (unsigned int)mode->v_visible);
}

int ft_mode_cmd(int argc, char **argv)
{
  esp_err_t err;
  esp_err_t err2;
  u32 mode;
  int ok;

  if (argc != 3)
  {
    printf("Usage:\r\n");
    printf("  ft mode <0-%lu>\r\n", (unsigned long)FT_MODE_MAX);
    ft_print_current_mode();
    return 1;
  }

  mode = ft_parse_num_arg(argv[2], "mode", &ok);
  if (!ok)
    return 1;

  if (mode > FT_MODE_MAX)
  {
    printf("Bad <mode>: %lu, allowed 0-%lu\r\n",
           (unsigned long)mode,
           (unsigned long)FT_MODE_MAX);
    return 1;
  }

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_set_mode((u8)mode);
  if (err == ESP_OK)
    err = ft_cp_reset();

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT mode failed: %d\r\n", (int)err);
    return 1;
  }

  printf("FT mode set: %lu\r\n", (unsigned long)mode);
  return 0;
}

u32 ft_parse_freq_arg_hz(const char *s, const char *name, int *ok)
{
  char buf[32];
  size_t n;
  char *endp = nullptr;
  double mhz;
  uint64_t hz;

  *ok = 0;

  if (!s)
  {
    printf("Missing <%s>\r\n", name);
    return 0;
  }

  n = strlen(s);
  if (n >= sizeof(buf))
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  memcpy(buf, s, n + 1);

  if (n && buf[n - 1] == ',')
    buf[n - 1] = 0;

  mhz = strtod(buf, &endp);
  if (!endp || *endp || mhz <= 0.0)
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  hz = (uint64_t)(mhz * 1000000.0 + 0.5);
  if (!hz || hz > 0xFFFFFFFFULL)
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  *ok = 1;
  return (u32)hz;
}

int ft_spi_cmd(int argc, char **argv)
{
  int ok;
  u32 width;

  if (argc < 3)
  {
    printf("Usage:\r\n");
    printf("  ft spi <1|2|4>\r\n");
    printf("Current FT SPI width: %u-bit\r\n", (unsigned int)ft_spi_width);
    return 0;
  }

  width = ft_parse_num_arg(argv[2], "width", &ok);
  if (!ok)
    return 1;

  if (width != 1 && width != 2 && width != 4)
  {
    printf("Bad <width>: %lu\r\n", (unsigned long)width);
    printf("Allowed: 1, 2, 4\r\n");
    return 1;
  }

  ft_spi_width = (u8)width;

  printf("FT SPI width set to %u-bit\r\n", (unsigned int)ft_spi_width);
  return 0;
}

int ft_freq_cmd(int argc, char **argv)
{
  esp_err_t err;
  esp_err_t err2;
  int ok;
  u32 actual_hz;

  if (argc > 3)
  {
    printf("Usage:\r\n");
    printf("  ft freq\r\n");
    printf("  ft freq <MHz>\r\n");
    return 1;
  }

  if (argc == 3)
  {
    ft_spi_freq_hz = ft_parse_freq_arg_hz(argv[2], "freq", &ok);
    if (!ok) return 1;

    ft_print_spi_freq("FT SPI req", ft_spi_freq_hz);
  }

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  actual_hz = spi_master_get_actual_freq_hz();
  if (actual_hz)
    ft_print_spi_freq("FT SPI actual", actual_hz);
  else
    printf("FT SPI actual: unavailable\r\n");

  err2 = ft_close_session();
  if (err2 != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err2);
    return 1;
  }

  return 0;
}

u32 ft_parse_num_arg(const char *s, const char *name, int *ok)
{
  char buf[32];
  size_t n;
  char *endp = nullptr;
  uint64_t v;

  *ok = 0;

  if (!s)
  {
    printf("Missing <%s>\r\n", name);
    return 0;
  }

  n = strlen(s);
  if (n >= sizeof(buf))
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  memcpy(buf, s, n + 1);

  if (n && buf[n - 1] == ',')
    buf[n - 1] = 0;

  v = strtoull(buf, &endp, 0);
  if (!endp || *endp)
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  if (v > 0xFFFFFFFFULL)
  {
    printf("Bad <%s>: %s\r\n", name, s);
    return 0;
  }

  *ok = 1;
  return (u32)v;
}

const char *ft_chip_type_name(const u8 chip_type[4])
{
  if (chip_type[0] != 0x08) return "Unknown";

  switch (chip_type[1])
  {
    case 0x10: return "FT810";
    case 0x11: return "FT811";
    case 0x12: return "FT812";
    case 0x13: return "FT813";
    case 0x15: return "BT815";
    case 0x16: return "BT816";
    case 0x17: return "BT817";
    case 0x18: return "BT818";
    default:   return "Unknown";
  }
}

void ft_print_detect(const u8 chip_id, const u8 chip_type[4], const char *datestamp)
{
  const char *type = ft_chip_type_name(chip_type);

  printf("Chip ID          : %02X\r\n", chip_id);
  printf("Chip type        : %s (%02X %02X %02X %02X)\r\n",
         type,
         chip_type[0], chip_type[1], chip_type[2], chip_type[3]);
  printf("Chip datestamp   : %s\r\n", datestamp);
}

esp_err_t ft_print_info()
{
  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  ft_read_detect(chip_id, chip_type, datestamp);

  u8 cpureset = ft_rreg8(FT_REG_CPURESET);
  u8 pclk = ft_rreg8(FT_REG_PCLK);
  u8 int_en = ft_rreg8(FT_REG_INT_EN);
  u8 int_mask = ft_rreg8(FT_REG_INT_MASK);
  u8 int_flags = ft_rreg8(FT_REG_INT_FLAGS);

  u16 hcycle = ft_rreg16(FT_REG_HCYCLE);
  u16 hoffset = ft_rreg16(FT_REG_HOFFSET);
  u16 hsize = ft_rreg16(FT_REG_HSIZE);
  u16 hsync0 = ft_rreg16(FT_REG_HSYNC0);
  u16 hsync1 = ft_rreg16(FT_REG_HSYNC1);
  u16 vcycle = ft_rreg16(FT_REG_VCYCLE);
  u16 voffset = ft_rreg16(FT_REG_VOFFSET);
  u16 vsize = ft_rreg16(FT_REG_VSIZE);
  u16 vsync0 = ft_rreg16(FT_REG_VSYNC0);
  u16 vsync1 = ft_rreg16(FT_REG_VSYNC1);
  u16 cmdb_space = ft_rreg16(FT_REG_CMDB_SPACE);

  u32 frames = ft_rreg32(FT_REG_FRAMES);
  u32 clock = ft_rreg32(FT_REG_CLOCK);
  u32 frequency = ft_rreg32(FT_REG_FREQUENCY);
  u8 spi_width = ft_rreg8(FT_REG_SPI_WIDTH);
  u32 esp_spi_clock = spi_master_get_actual_freq_hz();

  u16 hfront = hsync0;
  u16 hsync = hsync1 - hsync0;
  u16 hback = hoffset - hsync1;

  u16 vfront = vsync0 + 1;
  u16 vsync = vsync1 - vsync0;
  u16 vback = voffset - vsync1;

  u32 pixel_clock = 0;
  float line_rate_hz = 0.0f;
  float line_rate_khz = 0.0f;
  float frame_rate_hz = 0.0f;

  if (pclk)
  {
    pixel_clock = frequency / pclk;

    if (hcycle)
    {
      line_rate_hz = (float)pixel_clock / (float)hcycle;
      line_rate_khz = line_rate_hz / 1000.0f;
    }

    if (hcycle && vcycle)
      frame_rate_hz = line_rate_hz / (float)vcycle;
  }

  printf("FT812 information:\r\n");
  printf("  Chip ID          : %02X\r\n", chip_id);
  printf("  Chip type        : %s\r\n", ft_chip_type_name(chip_type));
  printf("  Datestamp        : %s\r\n", datestamp);

  printf("  PLL frequency    : %lu Hz\r\n", (unsigned long)frequency);
  printf("  Pixel clock      : %lu Hz\r\n", (unsigned long)pixel_clock);
  printf("  Pixel divider    : %u\r\n", (unsigned)pclk);

  printf("  Resolution       : %u x %u\r\n", (unsigned)hsize, (unsigned)vsize);
  printf("  Total frame      : %u x %u\r\n", (unsigned)hcycle, (unsigned)vcycle);
  printf("  Frame rate       : %.3f Hz\r\n", frame_rate_hz);
  printf("  Line rate        : %.3f kHz\r\n", line_rate_khz);
  printf("  Horizontal       : front=%u sync=%u back=%u active=%u\r\n", (unsigned)hfront, (unsigned)hsync, (unsigned)hback, (unsigned)hsize);
  printf("  Vertical         : front=%u sync=%u back=%u active=%u\r\n", (unsigned)vfront, (unsigned)vsync, (unsigned)vback, (unsigned)vsize);

  switch (spi_width & 3)
  {
    case FT_SPI_WIDTH_SINGLE: printf("  Bus width        : 1-bit SPI\r\n"); break;
    case FT_SPI_WIDTH_DUAL:   printf("  Bus width        : 2-bit SPI\r\n"); break;
    case FT_SPI_WIDTH_QUAD:   printf("  Bus width        : 4-bit SPI\r\n"); break;
    default:                  printf("  Bus width        : ? (%u)\r\n", (unsigned)(spi_width & 3)); break;
  }

  printf("  ESP SPI clock    : %lu Hz\r\n", (unsigned long)esp_spi_clock);

  printf("  Frame counter    : %lu\r\n", (unsigned long)frames);
  printf("  Clock counter    : %lu\r\n", (unsigned long)clock);

  printf("  Interrupts       : enable=%u mask=%02X flags=%02X\r\n", (unsigned)int_en, (unsigned)int_mask, (unsigned)int_flags);
  printf("  Command space    : %u\r\n", (unsigned)cmdb_space);
  printf("  CPU reset flags  : %u\r\n", (unsigned)cpureset);

  return ESP_OK;
}

esp_err_t ft_perf_run(void *buf, int is_read, u32 chunk_size, u32 total_size, int64_t *elapsed_us)
{
  if (!buf || !elapsed_us) return ESP_ERR_INVALID_ARG;
  if (!chunk_size || chunk_size > CHUNK_PAYLOAD) return ESP_ERR_INVALID_ARG;

  u32 left = total_size;
  int64_t t0 = esp_timer_get_time();

  while (left)
  {
    u32 n = left > chunk_size ? chunk_size : left;
    esp_err_t err;

    if (is_read)
      err = ft_read(buf, FT_RAM_G, n);
    else
      err = ft_write(buf, FT_RAM_G, n);

    if (err != ESP_OK) return err;

    left -= n;
  }

  *elapsed_us = esp_timer_get_time() - t0;
  return ESP_OK;
}

void ft_perf_fill_pattern(u8 *buf, u32 size)
{
  for (u32 i = 0; i < size; ++i)
    buf[i] = (u8)(i ^ (i >> 8) ^ 0x5A);
}

double ft_perf_mb_per_s(u32 bytes, int64_t elapsed_us)
{
  if (elapsed_us <= 0) return 0.0;

  return ((double)bytes * 1000000.0) / ((double)elapsed_us * 1024.0 * 1024.0);
}

int ft_perf_one(void *buf, const char *name, int is_read, u32 chunk_size, u32 total_size)
{
  if (chunk_size > CHUNK_PAYLOAD)
  {
    printf("%-20s : skipped, chunk=%lu > CHUNK_PAYLOAD=%u\r\n",
           name,
           (unsigned long)chunk_size,
           (unsigned int)CHUNK_PAYLOAD);
    return 0;
  }

  if (is_read)
  {
    esp_err_t err = ft_write(buf, FT_RAM_G, chunk_size);
    if (err != ESP_OK)
    {
      printf("%-20s : prep failed: %s (0x%x)\r\n",
             name,
             esp_err_to_name(err),
             (unsigned int)err);
      return 1;
    }
  }

  int64_t elapsed_us = 0;
  esp_err_t err = ft_perf_run(buf, is_read, chunk_size, total_size, &elapsed_us);
  if (err != ESP_OK)
  {
    printf("%-20s : failed: %s (0x%x)\r\n",
           name,
           esp_err_to_name(err),
           (unsigned int)err);
    return 1;
  }

  printf("%-20s : %7.3f MB/s  (%8llu us, chunk=%5lu)\r\n",
         name,
         ft_perf_mb_per_s(total_size, elapsed_us),
         (unsigned long long)elapsed_us,
         (unsigned long)chunk_size);

  return 0;
}

int ft_perf_suite(void *buf, const char *mem_name, u32 total_size)
{
  int failed = 0;
  char name[32];

  printf("\r\n[%s]\r\n", mem_name);

  snprintf(name, sizeof(name), "%s", "write max");
  if (ft_perf_one(buf, name, 0, CHUNK_PAYLOAD, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "write 1024");
  if (ft_perf_one(buf, name, 0, 1024, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "write 128");
  if (ft_perf_one(buf, name, 0, 128, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "write 16");
  if (ft_perf_one(buf, name, 0, 16, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "read max");
  if (ft_perf_one(buf, name, 1, CHUNK_PAYLOAD, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "read 1024");
  if (ft_perf_one(buf, name, 1, 1024, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "read 128");
  if (ft_perf_one(buf, name, 1, 128, total_size)) failed = 1;

  snprintf(name, sizeof(name), "%s", "read 16");
  if (ft_perf_one(buf, name, 1, 16, total_size)) failed = 1;

  return failed;
}

int ft_perf_cmd(int argc, char **argv)
{
  const u32 total_size = 1024 * 1024;
  esp_err_t err;
  esp_err_t err2;
  int failed = 0;
  u8 *perf_buf_int = nullptr;
  u8 *perf_buf_spiram = nullptr;

  (void)argv;

  if (argc != 2)
  {
    printf("Usage:\r\n");
    printf("  ft perf\r\n");
    return 1;
  }

  perf_buf_int = (u8*)heap_caps_malloc(CHUNK_PAYLOAD, MALLOC_CAP_INTERNAL);
  if (!perf_buf_int)
  {
    printf("FT perf internal buffer alloc failed, size=%u\r\n", (unsigned int)CHUNK_PAYLOAD);
    return 1;
  }

  perf_buf_spiram = (u8*)heap_caps_malloc(CHUNK_PAYLOAD, MALLOC_CAP_SPIRAM);
  if (!perf_buf_spiram)
  {
    printf("FT perf SPIRAM buffer alloc failed, size=%u\r\n", (unsigned int)CHUNK_PAYLOAD);
    heap_caps_free(perf_buf_int);
    return 1;
  }

  ft_perf_fill_pattern(perf_buf_int, CHUNK_PAYLOAD);
  ft_perf_fill_pattern(perf_buf_spiram, CHUNK_PAYLOAD);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    heap_caps_free(perf_buf_spiram);
    heap_caps_free(perf_buf_int);
    return 1;
  }

  printf("FT perf: total=%lu bytes, addr=0x%06lX, max_chunk=%u, spi=%u-bit, clk=%lu Hz\r\n",
         (unsigned long)total_size,
         (unsigned long)FT_RAM_G,
         (unsigned int)CHUNK_PAYLOAD,
         (unsigned int)ft_spi_width,
         (unsigned long)spi_master_get_actual_freq_hz());

  if (ft_perf_suite(perf_buf_int, "internal", total_size)) failed = 1;
  if (ft_perf_suite(perf_buf_spiram, "spiram", total_size)) failed = 1;

  err2 = ft_close_session();
  heap_caps_free(perf_buf_spiram);
  heap_caps_free(perf_buf_int);

  if (err2 != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err2);
    return 1;
  }

  return failed ? 1 : 0;
}

int ft_res_cmd(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  esp_err_t err;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_reset_chip();
  if (err != ESP_OK)
  {
    ft_close_session();
    printf("FT reset failed: %d\r\n", (int)err);
    return 1;
  }
  printf("\r\nFT reset done\r\n");

  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  ft_read_detect(chip_id, chip_type, datestamp);
  ft_print_detect(chip_id, chip_type, datestamp);

  err = ft_close_session();
  if (err != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_wreg_cmd(int argc, char **argv)
{
  esp_err_t err;
  u32 addr;
  u32 actual_addr;
  u32 value;
  int ok;

  if (argc != 4)
  {
    printf("Usage:\r\n");
    printf("  ft wreg <addr> <value>\r\n");
    return 1;
  }

  addr = ft_parse_num_arg(argv[2], "addr", &ok);
  if (!ok)
    return 1;

  value = ft_parse_num_arg(argv[3], "value", &ok);
  if (!ok)
    return 1;

  actual_addr = ((FT_RAM_REG >> 16) << 16) | (addr & 0xFFFFUL);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  ft_wreg32(addr, value);
  err = ft_close_session();

  if (err != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err);
    return 1;
  }

  printf("FT write: [0x%06lX] = 0x%08lX (%lu)\r\n",
         (unsigned long)actual_addr,
         (unsigned long)value,
         (unsigned long)value);

  return 0;
}

int ft_wr_cmd(int argc, char **argv)
{
  esp_err_t err;
  esp_err_t err2;
  u32 addr;
  u32 actual_addr;
  u32 value;
  u32 width = 0;
  u8 data[4];
  int ok;

  if (argc != 4)
  {
    printf("Usage:\r\n");
    printf("  ft wr <addr> <value>\r\n");
    printf("  ft wr8 <addr> <value>\r\n");
    printf("  ft wr16 <addr> <value>\r\n");
    printf("  ft wr32 <addr> <value>\r\n");
    return 1;
  }

  if (!strcmp(argv[1], "wr8"))
    width = 1;
  else if (!strcmp(argv[1], "wr16"))
    width = 2;
  else if (!strcmp(argv[1], "wr32"))
    width = 4;
  else if (strcmp(argv[1], "wr"))
    return 1;

  addr = ft_parse_num_arg(argv[2], "addr", &ok);
  if (!ok)
    return 1;

  value = ft_parse_num_arg(argv[3], "value", &ok);
  if (!ok)
    return 1;

  if (!width)
  {
    width = 1;
    if (value > 0xFFUL)
      width = 2;
    if (value > 0xFFFFUL)
      width = 4;
  }

  if (width == 1 && value > 0xFFUL)
  {
    printf("Bad <value>: 0x%08lX > 0xFF\r\n", (unsigned long)value);
    return 1;
  }

  if (width == 2 && value > 0xFFFFUL)
  {
    printf("Bad <value>: 0x%08lX > 0xFFFF\r\n", (unsigned long)value);
    return 1;
  }

  actual_addr = addr & 0x3FFFFFUL;
  data[0] = (u8)(value & 0xFF);
  if (width >= 2)
    wr16le(data, (u16)value);
  if (width == 4)
    wr32le(data, value);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_write(data, addr, width);
  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT write failed: %d\r\n", (int)err);
    return 1;
  }

  printf("FT write%lu: [0x%06lX] = 0x%08lX (%lu)\r\n",
         (unsigned long)(width * 8),
         (unsigned long)actual_addr,
         (unsigned long)value,
         (unsigned long)value);

  return 0;
}

int ft_info_cmd(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  esp_err_t err;
  esp_err_t err2;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  err = ft_read_detect(chip_id, chip_type, datestamp);
  if (err == ESP_OK)
  {
    printf("\r\n");
    err = ft_print_info();
  }

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT info read failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_dump_cmd(int argc, char **argv)
{
  esp_err_t err;
  esp_err_t err2;
  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  u8 b[256];
  u32 addr = 0;
  u32 actual_addr;
  int ok;

  if (argc > 3)
  {
    printf("Usage:\r\n");
    printf("  ft dump [addr]\r\n");
    return 1;
  }

  if (argc == 3)
  {
    addr = ft_parse_num_arg(argv[2], "addr", &ok);
    if (!ok)
      return 1;
  }

  actual_addr = addr & 0x3FFFFFUL;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_read_detect(chip_id, chip_type, datestamp);
  if (err != ESP_OK) printf("FT detect failed: %d\r\n", (int)err);

  printf("\r\nFT hexdump @ 0x%06lX:\r\n\r\n", (unsigned long)actual_addr);
  printf("           0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");
  err = ft_read(b, addr, sizeof(b));
  if (err == ESP_OK)
    hexdump(b, sizeof(b), actual_addr);

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT dump failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

esp_err_t ft_draw_calib_frame()
{
  const char *mode_txt = "1024x768@59Hz (64MHz)";
  esp_err_t err;
  esp_err_t err2;
  u16 i;
  u16 j;

  err = ft_open_session();
  if (err != ESP_OK)
    return err;

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);

    ft_ccmd_start(cmdl);
    ft_Dlstart();

    ft_VertexFormat(0);
    ft_ClearColorRGB(0, 0, 0);
    ft_Clear(1, 1, 1);

    ft_ColorRGB(255, 255, 255);
    ft_LineWidth(24);

    ft_Begin(FT_LINE_STRIP);
    ft_Vertex2f(0, 0);
    ft_Vertex2f(1023, 0);
    ft_Vertex2f(1023, 767);
    ft_Vertex2f(0, 767);
    ft_Vertex2f(0, 0);

    ft_Begin(FT_LINE_STRIP);
    ft_Vertex2f(0, 0);
    ft_Vertex2f(799, 0);
    ft_Vertex2f(799, 599);
    ft_Vertex2f(0, 599);
    ft_Vertex2f(0, 0);

    ft_Begin(FT_LINE_STRIP);
    ft_Vertex2f(0, 0);
    ft_Vertex2f(639, 0);
    ft_Vertex2f(639, 479);
    ft_Vertex2f(0, 479);
    ft_Vertex2f(0, 0);

    ft_Begin(FT_BITMAPS);
    ft_BitmapHandle(18);

    ft_ColorRGB(100, 220, 200);
    ft_print_string("Mode:", 10, 8);

    ft_ColorRGB(120, 100, 255);
    ft_print_string(mode_txt, 66, 8);

    ft_ColorRGB(120, 100, 255);
    ft_print_string("640x480", 572, 461);
    ft_print_string("800x600", 732, 581);
    ft_print_string("1024x768", 947, 749);

    ft_ColorA(255);
    ft_Begin(FT_POINTS);
    ft_PointSize(100 << 4);
    ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);

    ft_ColorRGB(255, 0, 0);
    ft_Vertex2f(rsin(80, 32768) + 320, rcos(80, 32768) + 240);

    ft_ColorRGB(0, 255, 0);
    ft_Vertex2f(rsin(80, 21845 + 32768) + 320, rcos(80, 21845 + 32768) + 240);

    ft_ColorRGB(0, 0, 255);
    ft_Vertex2f(rsin(80, 43690 - 32768) + 320, rcos(80, 43690 - 32768) + 240);

    ft_ClearColorA(32);
    ft_ColorMask(0, 0, 0, 1);
    ft_Clear(1, 1, 1);

    ft_BlendFunc(FT_ONE, FT_ONE);
    ft_ColorA(20);

    for (i = 120, j = 0; j <= 255; i -= 5, j += 20)
    {
      ft_PointSize(i << 4);
      ft_Vertex2f(320, 240);
    }

    ft_ColorRGB(0, 0, 0);
    ft_ColorMask(1, 1, 1, 1);
    ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_DST_ALPHA);
    ft_Begin(FT_RECTS);
    ft_Vertex2f(100, 50);
    ft_Vertex2f(539, 429);

    ft_Display();
    ft_Swap();

    err = ft_ccmd_write();
    if (err == ESP_OK)
      err = ft_cp_wait(1000);

    if (err == ESP_OK)
      err = ft_wait_swap(1000);
  }

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  return err;
}

int ft_demo_bases_cmd()
{
  esp_err_t err;

  const i16 x_pos = 350;
  const i16 y_pos = 250;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);

    while (1)
    {
      u32 clock = ft_rreg32(FT_REG_CLOCK);
      u32 frames = ft_rreg32(FT_REG_FRAMES);

      ft_ccmd_start(cmdl);

      ft_Dlstart();
      ft_VertexFormat(0);

      ft_ClearColorRGB(0, 0, 0);
      ft_ClearColorA(255);
      ft_Clear(1, 1, 1);

      ft_ColorA(255);
      ft_Begin(FT_POINTS);

      ft_PointSize(100 << 4);
      ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);

      ft_ColorRGB(255, 0, 0);
      ft_Vertex2f(rsin(80, 0) + x_pos, rcos(80, 0) + y_pos);

      ft_ColorRGB(0, 255, 0);
      ft_Vertex2f(rsin(80, 21845) + x_pos, rcos(80, 21845) + y_pos);

      ft_ColorRGB(0, 0, 255);
      ft_Vertex2f(rsin(80, 43690) + x_pos, rcos(80, 43690) + y_pos);

      ft_ClearColorA(32);
      ft_ColorMask(0, 0, 0, 1);
      ft_Clear(1, 1, 1);

      ft_BlendFunc(FT_ONE, FT_ONE);
      ft_ColorA(20);

      for (int i = 120, j = 0; j <= 255; i -= 5, j += 20)
      {
        ft_PointSize(i << 4);
        ft_Vertex2f(x_pos, y_pos);
      }

      ft_ColorRGB(0, 0, 0);
      ft_ColorMask(1, 1, 1, 1);
      ft_BlendFunc(FT_ONE_MINUS_DST_ALPHA, FT_DST_ALPHA);
      ft_Begin(FT_RECTS);
      ft_Vertex2f(0, 0);
      ft_Vertex2f(799, 599);

      ft_ColorA(255);
      ft_BlendFunc(FT_SRC_ALPHA, FT_ONE_MINUS_SRC_ALPHA);
      ft_RomFont(14, 34);

      ft_ColorRGB(255, 0, 0);
      ft_Number(100, 80, 14, 0, (i32)clock);

      ft_ColorRGB(0, 200, 240);
      ft_Number(100, 220, 14, 0, (i32)frames);

      ft_ColorRGB(0, 200, 0);
      ft_Text(100, 50, 31, 0, "Clocks:");
      ft_Text(100, 190, 31, 0, "Frames:");

      ft_ColorRGB(255, 120, 0);
      ft_Text(190, 500, 31, 0, "All your base are belong to us!");

      ft_Display();
      ft_Swap();

      err = ft_ccmd_write();
      if (err != ESP_OK)
      {
        printf("FT demo0 error: ft_ccmd_write() failed: %s (0x%x)\r\n",
          esp_err_to_name(err), (unsigned int)err);
        break;
      }

      err = ft_cp_wait(1000);
      if (err != ESP_OK)
      {
        printf("FT demo0 error: ft_cp_wait(1000) failed: %s (0x%x)\r\n",
          esp_err_to_name(err), (unsigned int)err);
        break;
      }

      err = ft_wait_swap(1000);
      if (err != ESP_OK)
      {
        printf("FT demo0 error: ft_wait_swap(1000) failed: %s (0x%x)\r\n",
          esp_err_to_name(err), (unsigned int)err);
        break;
      }

      char c;
      if (uart_read_bytes(UART_NUM_0, &c, 1, 0) > 0)
      {
        printf("FT demo stopped\r\n");
        break;
      }
    }
  }

  err = ft_close_session();
  if (err != ESP_OK)
  {
    printf("FT close failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

void ft_demo_frac_make_palette(u32 *pal)
{
  if (!pal) return;

  pal[0] = ft_argb32(255, 0, 0, 0);

  for (u32 i = 1; i < 256; i++)
  {
    u8 r;
    u8 g;
    u8 b;

    if (i < 64)
    {
      r = 0;
      g = 0;
      b = (u8)(i << 2);
    }

    else if (i < 128)
    {
      u8 t = (u8)((i - 64) << 2);
      r = 0;
      g = t;
      b = 255;
    }

    else if (i < 192)
    {
      u8 t = (u8)((i - 128) << 2);
      r = t;
      g = 255;
      b = (u8)(255 - t);
    }

    else
    {
      u8 t = (u8)((i - 192) << 2);
      r = 255;
      g = (u8)(255 - t);
      b = 0;
    }

    pal[i] = ft_argb32(255, r, g, b);
  }
}

typedef struct
{
  float *zr;
  float *zi;
  u8 *alive;
} FT_DEMO_FRAC_STATE;

int ft_demo_frac_state_init(FT_DEMO_FRAC_STATE *st, u8 *dst)
{
  u32 pixels = FT_DEMO_PIX_W * FT_DEMO_PIX_H;

  if (!st || !dst)
    return 0;

  memset(st, 0, sizeof(*st));

  st->zr = (float*)malloc_spiram(pixels * sizeof(float));
  st->zi = (float*)malloc_spiram(pixels * sizeof(float));
  st->alive = (u8*)malloc_spiram(pixels);

  if (!st->zr || !st->zi || !st->alive)
  {
    if (st->zr) free(st->zr);
    if (st->zi) free(st->zi);
    if (st->alive) free(st->alive);

    st->zr = nullptr;
    st->zi = nullptr;
    st->alive = nullptr;
    return 0;
  }

  memset(st->zr, 0, pixels * sizeof(float));
  memset(st->zi, 0, pixels * sizeof(float));
  memset(st->alive, 1, pixels);
  memset(dst, 0, pixels);

  return 1;
}

void ft_demo_frac_state_free(FT_DEMO_FRAC_STATE *st)
{
  if (!st) return;

  if (st->zr) free(st->zr);
  if (st->zi) free(st->zi);
  if (st->alive) free(st->alive);

  st->zr = nullptr;
  st->zi = nullptr;
  st->alive = nullptr;
}

void ft_demo_frac_render(FT_DEMO_FRAC_STATE *st, u8 *dst, u32 frame_no)
{
  static u8 offs = 0;

  for (u32 y = 0; y < FT_DEMO_PIX_H; y++)
  {
    for (u32 x = 0; x < FT_DEMO_PIX_W; x++)
    {
      u32 idx = y * FT_DEMO_PIX_W + x;
      dst[idx] = x * 256 / FT_DEMO_PIX_W + offs;
    }
  }

  offs++;
}

void ft_demo_frac_render1(FT_DEMO_FRAC_STATE *st, u8 *dst, u32 frame_no)
{
  if (!st || !dst) return;
  if (!frame_no) return;

  u32 iter_mark = frame_no;

  for (u32 y = 0; y < FT_DEMO_PIX_H; y++)
  {
    float ci = -1.5f + (float)y * 3.0f / (float)FT_DEMO_PIX_H;

    for (u32 x = 0; x < FT_DEMO_PIX_W; x++)
    {
      u32 idx = y * FT_DEMO_PIX_W + x;

      if (!st->alive[idx])
        continue;

      float cr = -2.0f + (float)x * 3.0f / (float)FT_DEMO_PIX_W;
      float zr = st->zr[idx];
      float zi = st->zi[idx];

      float zr_new = zr * zr - zi * zi + cr;
      float zi_new = 2.0f * zr * zi + ci;
      float mag2 = zr_new * zr_new + zi_new * zi_new;

      if (mag2 > 4.0f)
      {
        st->alive[idx] = 0;
        dst[idx] = (u8)iter_mark;
      }
      else
      {
        st->zr[idx] = zr_new;
        st->zi[idx] = zi_new;
      }
    }
  }
}

esp_err_t ft_demo_frac_upload_bitmap(const void *src, u32 ft_addr)
{
  return ft_write(src, ft_addr, FT_DEMO_PIX_SIZE);
}

esp_err_t ft_demo_frac_upload_bitmap_bg(const void *src, u32 ft_addr)
{
  return spi_master_write_buf_bg((u8)(((ft_addr >> 16) & 0x3F) | 0x80), (u16)ft_addr, src, FT_DEMO_PIX_SIZE);
}

esp_err_t ft_demo_frac_show_bitmap(u32 bmp_addr, u32 frame_no)
{
  esp_err_t err;
  u32 iters = frame_no;
  const u16 screen_w = 800;
  const u16 screen_h = 600;
  const i32 scale_x = ((i32)FT_DEMO_PIX_W << 8) / screen_w;
  const i32 scale_y = ((i32)FT_DEMO_PIX_H << 8) / screen_h;
  const u8 bitmap_filter = FT_NEAREST;
  // const u8 bitmap_filter = FT_BILINEAR;

  ft_ccmd_start(cmdl);

  ft_Dlstart();
  ft_VertexFormat(0);

  ft_ClearColorRGB(0, 0, 0);
  ft_ClearColorA(255);
  ft_Clear(1, 1, 1);

  ft_SaveContext();

  ft_BitmapHandle(0);
  ft_BitmapSource(bmp_addr);
  ft_BitmapLayout(FT_PALETTED8, (u16)FT_DEMO_PIX_W, (u16)FT_DEMO_PIX_H);
  ft_BitmapSize(bitmap_filter, FT_BORDER, FT_BORDER, screen_w, screen_h);
  ft_BitmapTransformA(scale_x);
  ft_BitmapTransformB(0);
  ft_BitmapTransformC(0);
  ft_BitmapTransformD(0);
  ft_BitmapTransformE(scale_y);
  ft_BitmapTransformF(0);

  ft_Begin(FT_BITMAPS);

  ft_BlendFunc(FT_ONE, FT_ZERO);

  ft_ColorMask(0, 0, 0, 1);
  ft_PaletteSource(FT_DEMO_PALETTE_ADDR + 3);
  ft_Vertex2ii(0, 0, 0, 0);

  ft_BlendFunc(FT_DST_ALPHA, FT_ONE_MINUS_DST_ALPHA);

  ft_ColorMask(1, 0, 0, 0);
  ft_PaletteSource(FT_DEMO_PALETTE_ADDR + 2);
  ft_Vertex2ii(0, 0, 0, 0);

  ft_ColorMask(0, 1, 0, 0);
  ft_PaletteSource(FT_DEMO_PALETTE_ADDR + 1);
  ft_Vertex2ii(0, 0, 0, 0);

  ft_ColorMask(0, 0, 1, 0);
  ft_PaletteSource(FT_DEMO_PALETTE_ADDR + 0);
  ft_Vertex2ii(0, 0, 0, 0);

  ft_RestoreContext();

  ft_ColorMask(1, 1, 1, 1);
  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE_MINUS_SRC_ALPHA);

  ft_ColorRGB(0, 255, 255);
  ft_Text(8, screen_h - 32, 18, 0, "Frame:");
  ft_Number(120, screen_h - 32, 18, 0, (i32)frame_no);

  ft_Text(8, screen_h - 16, 18, 0, "Iters:");
  ft_Number(120, screen_h - 16, 18, 0, (i32)iters);

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  return ESP_OK;
}

int ft_demo_frac_cmd()
{
  esp_err_t err;
  esp_err_t err2;
  FT_DEMO_FRAC_STATE st = {};
  u8 *render_bufs[2] =
  {
    nullptr,
    nullptr
  };
  u32 palette[256];
  u32 active_addr = FT_DEMO_PIX0_ADDR;
  u32 inactive_addr = FT_DEMO_PIX1_ADDR;
  u32 shown_frame = 1;
  u32 prepared_frame = 2;
  int upload_buf_idx = 1;

  render_bufs[0] = (u8*)heap_caps_malloc(FT_DEMO_PIX_SIZE, MALLOC_CAP_INTERNAL);
  render_bufs[1] = (u8*)heap_caps_malloc(FT_DEMO_PIX_SIZE, MALLOC_CAP_INTERNAL);

  if (!render_bufs[0] || !render_bufs[1])
  {
    printf("FT demo render buffer alloc failed, size=%lu\r\n", (unsigned long)FT_DEMO_PIX_SIZE);
    if (render_bufs[0]) free(render_bufs[0]);
    if (render_bufs[1]) free(render_bufs[1]);
    return 1;
  }

  if (!ft_demo_frac_state_init(&st, render_bufs[0]))
  {
    printf("FT demo state alloc failed\r\n");
    free(render_bufs[0]);
    free(render_bufs[1]);
    return 1;
  }

  memset(render_bufs[1], 0, FT_DEMO_PIX_SIZE);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    ft_demo_frac_state_free(&st);
    free(render_bufs[0]);
    free(render_bufs[1]);
    return 1;
  }

  ft_demo_frac_make_palette(palette);
  err = ft_write(palette, FT_DEMO_PALETTE_ADDR, FT_DEMO_PALETTE_SIZE);

  if (err == ESP_OK)
  {
    ft_demo_frac_render(&st, render_bufs[0], shown_frame);
    ft_demo_frac_render(&st, render_bufs[1], prepared_frame);
    err = ft_demo_frac_upload_bitmap_bg(render_bufs[0], active_addr);
  }

  if (err == ESP_OK)
    err = spi_master_bg_wait_done(portMAX_DELAY);

  if (err == ESP_OK)
    err = ft_demo_frac_show_bitmap(active_addr, shown_frame);

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);
    err = profiler_start();

    if (err == ESP_OK)
    {
      while (1)
      {
        PROFILER_LINE line = {};
        int render_buf_idx = upload_buf_idx ^ 1;
        u32 future_frame = prepared_frame + 1;
        int64_t t0 = esp_timer_get_time();

        while (1)
        {
          if (ft_rreg8(FT_REG_INT_FLAGS) & FT_INT_SWAP)
            break;

          if (((esp_timer_get_time() - t0) / 1000) >= 1000)
          {
            err = ESP_ERR_TIMEOUT;
            break;
          }

          vTaskDelay(pdMS_TO_TICKS(1));
        }
        if (err != ESP_OK) break;

        line.ts_loop_start = profiler_now();

        line.ts_upload_start = profiler_now();
        err = ft_demo_frac_upload_bitmap_bg(render_bufs[upload_buf_idx], inactive_addr);
        if (err != ESP_OK) break;

        line.ts_render_start = profiler_now();
        ft_demo_frac_render(&st, render_bufs[render_buf_idx], future_frame);
        line.ts_render_end = profiler_now();

        err = spi_master_bg_wait_done(portMAX_DELAY);
        line.ts_upload_done = profiler_now();
        if (err != ESP_OK) break;

        err = ft_demo_frac_show_bitmap(inactive_addr, prepared_frame);
        if (err != ESP_OK) break;

        profiler_push(&line);

        shown_frame = prepared_frame;
        prepared_frame = future_frame;
        upload_buf_idx = render_buf_idx;

        u32 tmp = active_addr;
        active_addr = inactive_addr;
        inactive_addr = tmp;

        char c;
        if (uart_read_bytes(UART_NUM_0, &c, 1, 0) > 0)
        {
          printf("FT demo stopped\r\n");
          break;
        }
      }

      profiler_stop();
    }
    else
      printf("Profiler start failed: %s (0x%x)\r\n", esp_err_to_name(err), (unsigned int)err);
  }

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  ft_demo_frac_state_free(&st);
  free(render_bufs[0]);
  free(render_bufs[1]);

  if (err != ESP_OK)
  {
    printf("FT demo_frac failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

u8 ft_text_cur_ink = 15;
u8 ft_text_cur_paper = 0;
u8 *ft_text_chars_addr = nullptr;
u8 *ft_text_attrs_addr = nullptr;

u8 *ft_text_attr_mode_upload = nullptr;
u8 *ft_text_attr_mode_blank_chars = nullptr;
u8 *ft_text_attr_mode_blank_attrs = nullptr;
u16 ft_text_attr_mode_cols = FT_DEMO_TEXT_COLS;
u16 ft_text_attr_mode_rows = FT_DEMO_TEXT_ROWS;
bool ft_text_attr_mode_open = false;
bool ft_text_attr_mode_palette_dirty = false;

void ft_text_set_attr(u8 ink, u8 paper)
{
  ft_text_cur_ink = (u8)(ink & 15U);
  ft_text_cur_paper = (u8)(paper & 15U);
}

void ft_text_set_addrs(u8 *chars, u8 *attrs)
{
  ft_text_chars_addr = chars;
  ft_text_attrs_addr = attrs;
}

u8 ft_text_attr(u8 ink, u8 paper)
{
  return (u8)(((paper & 15U) << 4) | (ink & 15U));
}

void ft_text_clear(u8 ch, u8 attr)
{
  if (!ft_text_chars_addr || !ft_text_attrs_addr) return;

  for (u32 i = 0; i < ((u32)FT_DEMO_TEXT_COLS * (u32)FT_DEMO_TEXT_ROWS); i++)
  {
    ft_text_chars_addr[i] = ch;
    ft_text_attrs_addr[i] = attr;
  }
}

void ft_text_put_raw(int x, int y, u8 ch, u8 attr)
{
  u32 off;

  if (!ft_text_chars_addr || !ft_text_attrs_addr) return;
  if (x < 0 || x >= FT_DEMO_TEXT_COLS) return;
  if (y < 0 || y >= FT_DEMO_TEXT_ROWS) return;

  off = (u32)y * FT_DEMO_TEXT_COLS + (u32)x;
  ft_text_chars_addr[off] = ch;
  ft_text_attrs_addr[off] = attr;
}

void ft_text_putc(int x, int y, u8 ch)
{
  ft_text_put_raw(x, y, ch, ft_text_attr(ft_text_cur_ink, ft_text_cur_paper));
}

void ft_text_fill_rect(int x, int y, int w, int h, u8 ch, u8 attr)
{
  if (w <= 0 || h <= 0) return;

  for (int yy = 0; yy < h; yy++)
  {
    int py = y + yy;

    if (py < 0 || py >= FT_DEMO_TEXT_ROWS)
      continue;

    for (int xx = 0; xx < w; xx++)
      ft_text_put_raw(x + xx, py, ch, attr);
  }
}

void ft_text_fill_paper(int x, int y, int w, int h, u8 paper)
{
  ft_text_fill_rect(x, y, w, h, ' ', ft_text_attr(15, paper));
}

void ft_text_draw_box2(int x, int y, int w, int h)
{
  if (w < 2 || h < 2) return;

  ft_text_putc(x,         y,         0xC9);
  ft_text_putc(x + w - 1, y,         0xBB);
  ft_text_putc(x,         y + h - 1, 0xC8);
  ft_text_putc(x + w - 1, y + h - 1, 0xBC);

  for (int xx = 1; xx < (w - 1); xx++)
  {
    ft_text_putc(x + xx, y,         0xCD);
    ft_text_putc(x + xx, y + h - 1, 0xCD);
  }

  for (int yy = 1; yy < (h - 1); yy++)
  {
    ft_text_putc(x,         y + yy, 0xBA);
    ft_text_putc(x + w - 1, y + yy, 0xBA);
  }
}

void ft_text_puts(int x, int y, const char *s)
{
  if (!ft_text_chars_addr || !ft_text_attrs_addr || !s) return;
  if (y < 0 || y >= FT_DEMO_TEXT_ROWS) return;

  while (*s && x < FT_DEMO_TEXT_COLS)
  {
    if (x >= 0)
      ft_text_putc(x, y, (u8)*s);

    x++;
    s++;
  }
}

char ft_hex_digit(u8 v)
{
  v &= 15;

  if (v < 10)
    return (char)('0' + v);

  return (char)('A' + (v - 10));
}

void ft_text_put_hex1(int x, int y, u8 v)
{
  ft_text_putc(x, y, (u8)ft_hex_digit(v));
}

void ft_text_to_attr_render_size(u8 *gfx, const u8 *chars, u8 *color, const u8 *attrs, u16 cols, u16 rows)
{
  u32 column_pitch = 256;
  u32 row_pitch = 256;
  int col_count;
  int row_count;

  if (!gfx || !chars) return;

  if (cols == 0) cols = FT_DEMO_TEXT_COLS;
  if (rows == 0) rows = FT_DEMO_TEXT_ROWS;
  if (cols > FT_TEXT_ATTR_MAX_COLS) cols = FT_TEXT_ATTR_MAX_COLS;
  if (rows > FT_TEXT_ATTR_MAX_ROWS) rows = FT_TEXT_ATTR_MAX_ROWS;

  col_count = (int)cols;
  row_count = (int)rows;

  memset(gfx, 0, (size_t)FT_TEXT_ATTR_GFX_SIZE_FOR(cols, rows));

  for (int cy = 0; cy < row_count; cy++)
  {
    u32 group = (u32)cy / FT_TEXT_ATTR_GROUP_ROWS;
    u32 local_y = (u32)cy % FT_TEXT_ATTR_GROUP_ROWS;

    for (int cx = 0; cx < col_count; cx++)
    {
      u32 col_index = group * (u32)cols + (u32)cx;
      u8 *col_dst = gfx + col_index * column_pitch;
      u8 ch = chars[(u32)cy * (u32)cols + (u32)cx];

      for (int gy = 0; gy < 8; gy++)
        col_dst[local_y * 8U + (u32)gy] = code_866_fnt[(u32)ch * 8U + (u32)gy];
    }
  }

  if (!color || !attrs) return;

  memset(color, 0, (size_t)((u32)rows * row_pitch));

  for (int cy = 0; cy < row_count; cy++)
  {
    u8 *row_dst = color + (u32)cy * row_pitch;

    for (int cx = 0; cx < col_count; cx++)
      row_dst[cx] = attrs[(u32)cy * (u32)cols + (u32)cx];
  }
}

void ft_text_to_attr_render(u8 *gfx, const u8 *chars, u8 *color, const u8 *attrs)
{
  ft_text_to_attr_render_size(gfx, chars, color, attrs, FT_DEMO_TEXT_COLS, FT_DEMO_TEXT_ROWS);
}

void ft_text_to_4bpp_render(u8 *dst, const u8 *chars, const u8 *attrs)
{
  if (!dst || !chars || !attrs) return;

  for (int cy = 0; cy < FT_DEMO_TEXT_ROWS; cy++)
  {
    for (int gy = 0; gy < 8; gy++)
    {
      u8 *drow = dst + (u32)(cy * 8 + gy) * ((u32)FT_DEMO_TEXT_COLS * 4U);

      for (int cx = 0; cx < FT_DEMO_TEXT_COLS; cx++)
      {
        u32 off = (u32)cy * FT_DEMO_TEXT_COLS + (u32)cx;
        u8 ch = chars[off];
        u8 attr = attrs[off];
        u8 ink = (u8)(attr & 15U);
        u8 paper = (u8)((attr >> 4) & 15U);
        u8 bits = code_866_fnt[(u32)ch * 8U + (u32)gy];
        u8 *dp = drow + (u32)cx * 4U;

        for (int i = 0; i < 4; i++)
        {
          u8 p0 = (bits & 0x80U) ? ink : paper;
          bits <<= 1;
          u8 p1 = (bits & 0x80U) ? ink : paper;
          bits <<= 1;
          dp[i] = (u8)((p0 << 4) | p1);
        }
      }
    }
  }
}

u16 ft_attr_to_rgb565(u8 c)
{
  u8 level = (c & 8U) ? 255U : 191U;
  u8 b = (c & 1U) ? level : 0U;
  u8 r = (c & 2U) ? level : 0U;
  u8 g = (c & 4U) ? level : 0U;

  return (u16)(((u16)(r & 0xF8U) << 8) | ((u16)(g & 0xFCU) << 3) | ((u16)b >> 3));
}

void ft_text_make_palettes(u16 *pal_hi, u16 *pal_lo)
{
  u16 pal4[16];

  if (!pal_hi || !pal_lo) return;

  for (u32 c = 0; c < 16; c++)
  {
    u8 intensity = (c & 8U) ? 85U : 0U;
    u8 b = (c & 1U) ? (u8)(intensity + 170U) : intensity;
    u8 r = (c & 2U) ? (u8)(intensity + 170U) : intensity;
    u8 g = (c & 4U) ? (u8)(intensity + 170U) : intensity;

    pal4[c] = (u16)(((u16)(r & 0xF8U) << 8) | ((u16)(g & 0xFCU) << 3) | ((u16)b >> 3));
  }

  for (u32 i = 0; i < 256; i++)
  {
    pal_hi[i] = pal4[i >> 4];
    pal_lo[i] = pal4[i & 15U];
  }
}

esp_err_t ft_text_palette_init_at(u32 pal_hi_addr, u32 pal_lo_addr)
{
  esp_err_t err;
  u16 pal_hi[256];
  u16 pal_lo[256];

  ft_text_make_palettes(pal_hi, pal_lo);

  err = ft_write(pal_hi, pal_hi_addr, sizeof(pal_hi));
  if (err != ESP_OK) return err;

  err = ft_write(pal_lo, pal_lo_addr, sizeof(pal_lo));
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t ft_text_palette_init()
{
  return ft_text_palette_init_at(FT_DEMO_PAL4_PAL_HI_ADDR, FT_DEMO_PAL4_PAL_LO_ADDR);
}

esp_err_t ft_text_attr_mode_palette_init()
{
  esp_err_t err;

  err = ft_text_palette_init_at(
    FT_TEXT_ATTR_PAL_HI_ADDR_FOR(ft_text_attr_mode_cols, ft_text_attr_mode_rows),
    FT_TEXT_ATTR_PAL_LO_ADDR_FOR(ft_text_attr_mode_cols, ft_text_attr_mode_rows));
  if (err != ESP_OK) return err;

  ft_text_attr_mode_palette_dirty = false;
  return ESP_OK;
}

esp_err_t ft_4bpp_show()
{
  esp_err_t err;
  u16 bitmap_w_bytes = (u16)(FT_DEMO_TEXT_COLS * 4);
  u16 bitmap_h = (u16)(FT_DEMO_TEXT_ROWS * 8);
  u16 screen_w = (u16)(FT_DEMO_TEXT_COLS * 8);
  u16 screen_h = (u16)(FT_DEMO_TEXT_ROWS * 16);
  u16 screen_x = (u16)((800 - screen_w) / 2);
  u16 screen_y = (u16)((600 - screen_h) / 2);

  ft_ccmd_start(cmdl);

  ft_Dlstart();

  ft_ClearColorRGB(0, 0, 32);
  ft_ClearColorA(255);
  ft_ClearStencil(0);
  ft_Clear(1, 1, 1);

  ft_BitmapHandle(0);
  ft_BitmapSource(FT_DEMO_PAL4_PIX_ADDR);
  ft_BitmapLayout(FT_PALETTED565, bitmap_w_bytes, bitmap_h);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, screen_w, screen_h);

  ft_BitmapHandle(1);
  ft_BitmapSource(FT_DEMO_PAL4_MASK_ADDR);
  ft_BitmapLayout(FT_L4, 1, 1);
  ft_BitmapSize(FT_NEAREST, FT_REPEAT, FT_REPEAT, screen_w, screen_h);

  ft_BitmapTransformB(0);
  ft_BitmapTransformC(0);
  ft_BitmapTransformD(0);
  ft_BitmapTransformF(0);

  ft_BitmapTransformA(256);
  ft_BitmapTransformE(256);

  ft_ColorMask(0, 0, 0, 0);
  ft_StencilMask(255);
  ft_AlphaFunc(FT_GREATER, 0);
  ft_StencilFunc(FT_ALWAYS, 1, 255);
  ft_StencilOp(FT_KEEP, FT_REPLACE);
  ft_Begin(FT_BITMAPS);
  ft_Vertex2ii(screen_x, screen_y, 1, 0);

  ft_ColorMask(1, 1, 1, 0);
  ft_StencilMask(0);
  ft_AlphaFunc(FT_ALWAYS, 0);
  ft_BlendFunc(FT_ONE, FT_ZERO);

  ft_BitmapTransformA(128);
  ft_BitmapTransformE(128);

  ft_StencilFunc(FT_EQUAL, 1, 255);
  ft_PaletteSource(FT_DEMO_PAL4_PAL_HI_ADDR);
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_StencilFunc(FT_EQUAL, 0, 255);
  ft_PaletteSource(FT_DEMO_PAL4_PAL_LO_ADDR);
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t ft_attr_show()
{
  esp_err_t err;
  u16 bitmap_h = FT_DEMO_TEXT_ROWS;
  u16 screen_w = (u16)(FT_DEMO_TEXT_COLS * 8);
  u16 screen_h = (u16)(FT_DEMO_TEXT_ROWS * 16);
  u16 screen_x = (u16)((800 - screen_w) / 2);
  u16 screen_y = (u16)((600 - screen_h) / 2);

  ft_ccmd_start(cmdl);

  ft_Dlstart();

  ft_ClearColorRGB(0, 0, 32);
  ft_ClearColorA(255);
  ft_ClearStencil(0);
  ft_Clear(1, 1, 1);

  ft_BitmapHandle(0);
  ft_BitmapSource(FT_DEMO_ATTR_ADDR);
  ft_BitmapLayout(FT_PALETTED565, 256, bitmap_h);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, screen_w, screen_h);

  ft_BitmapHandle(1);
  ft_BitmapSource(FT_DEMO_PIX_ADDR(0));
  ft_BitmapLayout(FT_L1, 1, 256);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 8, screen_h);

  ft_BitmapTransformB(0);
  ft_BitmapTransformC(0);
  ft_BitmapTransformD(0);
  ft_BitmapTransformF(0);

  ft_BitmapTransformA(256);
  ft_BitmapTransformE(128);

  ft_ColorMask(0, 0, 0, 0);
  ft_StencilMask(255);
  ft_AlphaFunc(FT_GREATER, 0);
  ft_StencilFunc(FT_ALWAYS, 1, 255);
  ft_StencilOp(FT_KEEP, FT_REPLACE);

  ft_Begin(FT_BITMAPS);

  ft_VertexTranslateX((i32)screen_x * 16);
  ft_VertexTranslateY((i32)screen_y * 16);

  for (int cx = 0; cx < 64; cx++)
    ft_Vertex2ii((u16)(cx * 8), 0, 1, (u8)cx);

  ft_VertexTranslateX((i32)(screen_x + 64 * 8) * 16);

  for (int cx = 64; cx < FT_DEMO_TEXT_COLS; cx++)
    ft_Vertex2ii((u16)((cx - 64) * 8), 0, 1, (u8)cx);

  ft_VertexTranslateX(0);
  ft_VertexTranslateY(0);

  ft_ColorMask(1, 1, 1, 0);
  ft_StencilMask(0);
  ft_AlphaFunc(FT_ALWAYS, 0);
  ft_BlendFunc(FT_ONE, FT_ZERO);

  ft_BitmapTransformA(32);
  ft_BitmapTransformE(16);

  ft_StencilFunc(FT_EQUAL, 0, 255);
  ft_PaletteSource(FT_DEMO_PAL4_PAL_HI_ADDR);
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_StencilFunc(FT_EQUAL, 1, 255);
  ft_PaletteSource(FT_DEMO_PAL4_PAL_LO_ADDR);
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

void ft_text_attr_mode_calc_size(u16 *cols, u16 *rows, u16 *visible_w, u16 *visible_h)
{
  u16 w = 640;
  u16 h = 480;
  u16 c;
  u16 r;

  if (ft_current_mode >= 0 && ft_current_mode < FT_MODE_MAX)
  {
    w = ft_modes[ft_current_mode].h_visible;
    h = ft_modes[ft_current_mode].v_visible;
  }

  c = (u16)(w / 8U);
  r = (u16)(h / 16U);

  if (c == 0) c = FT_DEMO_TEXT_COLS;
  if (r == 0) r = FT_DEMO_TEXT_ROWS;
  if (c > FT_TEXT_ATTR_MAX_COLS) c = FT_TEXT_ATTR_MAX_COLS;
  if (r > FT_TEXT_ATTR_MAX_ROWS) r = FT_TEXT_ATTR_MAX_ROWS;

  if (cols) *cols = c;
  if (rows) *rows = r;
  if (visible_w) *visible_w = w;
  if (visible_h) *visible_h = h;
}

void ft_text_attr_mode_get_size(uint16_t *cols, uint16_t *rows)
{
  u16 c;
  u16 r;

  if (ft_text_attr_mode_open)
  {
    c = ft_text_attr_mode_cols;
    r = ft_text_attr_mode_rows;
  }
  else
    ft_text_attr_mode_calc_size(&c, &r, nullptr, nullptr);

  if (cols) *cols = c;
  if (rows) *rows = r;
}

esp_err_t ft_text_attr_mode_show()
{
  esp_err_t err;
  u16 cols = ft_text_attr_mode_cols;
  u16 rows = ft_text_attr_mode_rows;
  u16 bitmap_h = rows;
  u16 text_w = (u16)(cols * 8U);
  u16 text_h = (u16)(rows * 16U);
  u16 visible_w;
  u16 visible_h;
  u16 screen_x = 0;
  u16 screen_y = 0;
  int col_count = (int)cols;

  ft_text_attr_mode_calc_size(nullptr, nullptr, &visible_w, &visible_h);

  if (ft_text_attr_mode_palette_dirty)
  {
    err = ft_text_attr_mode_palette_init();
    if (err != ESP_OK) return err;
  }

  if (visible_w > text_w)
    screen_x = (u16)((visible_w - text_w) / 2U);

  if (visible_h > text_h)
    screen_y = (u16)((visible_h - text_h) / 2U);

  ft_ccmd_start(cmdl);

  ft_Dlstart();

  ft_ClearColorRGB(0, 0, 32);
  ft_ClearColorA(255);
  ft_ClearStencil(0);
  ft_Clear(1, 1, 1);

  ft_BitmapHandle(0);
  ft_BitmapSource(FT_DEMO_ATTR_ADDR_FOR(cols, rows));
  ft_BitmapLayout(FT_PALETTED565, 256, bitmap_h);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, text_w, text_h);

  ft_BitmapHandle(1);

  ft_BitmapTransformB(0);
  ft_BitmapTransformC(0);
  ft_BitmapTransformD(0);
  ft_BitmapTransformF(0);

  ft_BitmapTransformA(256);
  ft_BitmapTransformE(128);

  ft_ColorMask(0, 0, 0, 0);
  ft_StencilMask(255);
  ft_AlphaFunc(FT_GREATER, 0);
  ft_StencilFunc(FT_ALWAYS, 1, 255);
  ft_StencilOp(FT_KEEP, FT_REPLACE);

  ft_Begin(FT_BITMAPS);

  for (u32 group = 0; group < FT_TEXT_ATTR_GROUPS_FOR(rows); group++)
  {
    u32 row0 = group * FT_TEXT_ATTR_GROUP_ROWS;
    u32 group_rows = (u32)rows - row0;
    u16 group_text_h;

    if (group_rows > FT_TEXT_ATTR_GROUP_ROWS)
      group_rows = FT_TEXT_ATTR_GROUP_ROWS;

    group_text_h = (u16)(group_rows * 16U);

    ft_BitmapLayout(FT_L1, 1, 256);
    ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 8, group_text_h);

    for (int start = 0; start < col_count; start += 64)
    {
      int count = col_count - start;
      if (count > 64) count = 64;

      ft_BitmapSource(FT_DEMO_PIX_GROUP_ADDR(group, start, cols));
      ft_VertexTranslateX((i32)(screen_x + start * 8) * 16);
      ft_VertexTranslateY((i32)(screen_y + row0 * 16U) * 16);

      for (int cx = 0; cx < count; cx++)
        ft_Vertex2ii((u16)(cx * 8), 0, 1, (u8)cx);
    }
  }

  ft_VertexTranslateX(0);
  ft_VertexTranslateY(0);

  ft_ColorMask(1, 1, 1, 0);
  ft_StencilMask(0);
  ft_AlphaFunc(FT_ALWAYS, 0);
  ft_BlendFunc(FT_ONE, FT_ZERO);

  ft_BitmapTransformA(32);
  ft_BitmapTransformE(16);

  ft_BitmapHandle(0);
  ft_BitmapSource(FT_DEMO_ATTR_ADDR_FOR(cols, rows));

  ft_StencilFunc(FT_EQUAL, 0, 255);
  ft_PaletteSource(FT_TEXT_ATTR_PAL_HI_ADDR_FOR(cols, rows));
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_StencilFunc(FT_EQUAL, 1, 255);
  ft_PaletteSource(FT_TEXT_ATTR_PAL_LO_ADDR_FOR(cols, rows));
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t ft_text_attr_mode_present(const u8 *chars, const u8 *attrs)
{
  esp_err_t err;
  u32 gfx_size;
  u32 upload_size;

  if (!chars || !attrs) return ESP_ERR_INVALID_ARG;
  if (!ft_text_attr_mode_open) return ESP_ERR_INVALID_STATE;
  if (!ft_text_attr_mode_upload) return ESP_ERR_INVALID_STATE;

  gfx_size = FT_TEXT_ATTR_GFX_SIZE_FOR(ft_text_attr_mode_cols, ft_text_attr_mode_rows);
  upload_size = FT_TEXT_ATTR_UPLOAD_SIZE_FOR(ft_text_attr_mode_cols, ft_text_attr_mode_rows);

  u8 *gfx = ft_text_attr_mode_upload;
  u8 *color = ft_text_attr_mode_upload + gfx_size;

  ft_text_to_attr_render_size(gfx, chars, color, attrs, ft_text_attr_mode_cols, ft_text_attr_mode_rows);

  err = ft_write(ft_text_attr_mode_upload, FT_DEMO_PIX_ADDR(0), upload_size);
  if (err != ESP_OK) return err;

  if (ft_text_attr_mode_palette_dirty)
    return ft_text_attr_mode_palette_init();

  return ESP_OK;
}

esp_err_t ft_text_attr_mode_clear(u8 ch, u8 attr)
{
  u32 text_cells = (u32)ft_text_attr_mode_cols * (u32)ft_text_attr_mode_rows;

  if (!ft_text_attr_mode_open) return ESP_ERR_INVALID_STATE;
  if (!ft_text_attr_mode_blank_chars || !ft_text_attr_mode_blank_attrs) return ESP_ERR_INVALID_STATE;

  if (ch == 0)
    ch = ' ';

  memset(ft_text_attr_mode_blank_chars, ch, text_cells);
  memset(ft_text_attr_mode_blank_attrs, attr, text_cells);

  return ft_text_attr_mode_present(ft_text_attr_mode_blank_chars, ft_text_attr_mode_blank_attrs);
}

void ft_text_attr_mode_free()
{
  if (ft_text_attr_mode_upload) free(ft_text_attr_mode_upload);
  if (ft_text_attr_mode_blank_chars) free(ft_text_attr_mode_blank_chars);
  if (ft_text_attr_mode_blank_attrs) free(ft_text_attr_mode_blank_attrs);

  ft_text_attr_mode_upload = nullptr;
  ft_text_attr_mode_blank_chars = nullptr;
  ft_text_attr_mode_blank_attrs = nullptr;
  ft_text_attr_mode_cols = FT_DEMO_TEXT_COLS;
  ft_text_attr_mode_rows = FT_DEMO_TEXT_ROWS;
  ft_text_attr_mode_palette_dirty = false;
}

esp_err_t ft_text_attr_mode_begin()
{
  esp_err_t err;
  u32 text_cells;
  u32 upload_size;

  if (ft_text_attr_mode_open) return ESP_OK;

  ft_text_attr_mode_calc_size(&ft_text_attr_mode_cols, &ft_text_attr_mode_rows, nullptr, nullptr);
  text_cells = (u32)ft_text_attr_mode_cols * (u32)ft_text_attr_mode_rows;
  upload_size = FT_TEXT_ATTR_UPLOAD_SIZE_FOR(ft_text_attr_mode_cols, ft_text_attr_mode_rows);

  ft_text_attr_mode_upload = (u8*)heap_caps_malloc(upload_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  ft_text_attr_mode_blank_chars = (u8*)heap_caps_malloc(text_cells, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  ft_text_attr_mode_blank_attrs = (u8*)heap_caps_malloc(text_cells, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (!ft_text_attr_mode_upload || !ft_text_attr_mode_blank_chars || !ft_text_attr_mode_blank_attrs)
  {
    ft_text_attr_mode_free();
    return ESP_ERR_NO_MEM;
  }

  err = ft_open_session();
  if (err != ESP_OK)
  {
    ft_text_attr_mode_free();
    return err;
  }

  ft_text_attr_mode_open = true;

  err = ft_text_attr_mode_palette_init();
  if (err == ESP_OK)
    err = ft_text_attr_mode_clear(' ', 0x0f);

  if (err == ESP_OK)
    err = ft_text_attr_mode_show();

  if (err == ESP_OK)
    err = ft_wait_swap(1000);

  if (err != ESP_OK)
  {
    ft_text_attr_mode_open = false;
    ft_close_session();
    ft_text_attr_mode_free();
    return err;
  }

  return ESP_OK;
}

void ft_text_attr_mode_end()
{
  if (ft_text_attr_mode_open)
    ft_close_session();

  ft_text_attr_mode_open = false;
  ft_text_attr_mode_free();
}

void ft_demo_text_fill(const char *title, const char *subtitle)
{
  const int left_x = 2;
  const int right_x = 42;

  if (!ft_text_chars_addr || !ft_text_attrs_addr) return;

  ft_text_clear(' ', ft_text_attr(15, 0));
  ft_text_fill_paper(0, 0, FT_DEMO_TEXT_COLS, FT_DEMO_TEXT_ROWS, 0);

  ft_text_set_attr(15, 0);
  ft_text_draw_box2(0, 0, FT_DEMO_TEXT_COLS, FT_DEMO_TEXT_ROWS);

  ft_text_set_attr(14, 0);
  ft_text_puts(left_x, 1, title);

  ft_text_set_attr(11, 0);
  ft_text_puts(left_x, 2, subtitle);

  {
    int x0 = left_x;
    int y0 = 11;

    ft_text_set_attr(15, 0);
    for (int ink = 0; ink < 16; ink++)
      ft_text_put_hex1(x0 + 4 + ink * 2, y0, (u8)ink);

    for (int paper = 0; paper < 16; paper++)
    {
      ft_text_set_attr(15, (u8)paper);
      ft_text_put_hex1(x0 + 1, y0 + 1 + paper, (u8)paper);

      for (int ink = 0; ink < 16; ink++)
      {
        ft_text_set_attr((u8)ink, (u8)paper);
        ft_text_putc(x0 + 4 + ink * 2, y0 + 1 + paper, 'A');
      }
    }
  }

  {
    int x0 = right_x;
    int y0 = 11;

    ft_text_set_attr(15, 0);
    for (int col = 0; col < 16; col++)
      ft_text_put_hex1(x0 + 3 + col * 2, y0, (u8)col);

    for (int row = 0; row < 16; row++)
    {
      ft_text_put_hex1(x0 + 1, y0 + 1 + row, (u8)row);

      ft_text_set_attr(7, 0);
      for (int col = 0; col < 16; col++)
        ft_text_putc(x0 + 3 + col * 2, y0 + 1 + row, (u8)(row * 16 + col));

      ft_text_set_attr(15, 0);
    }
  }
}

esp_err_t ft_rgb888_640x480_load_mode(u32 rgb_addr, bool stretch, bool swap);
esp_err_t ft_rgb888_640x480_show_mode(u32 rgb_addr, bool stretch);
esp_err_t ft_dxp_show_mode(const FT_DXP_INFO *info);

int ft_jpg_ext_match(const char *name)
{
  const char *dot = strrchr(name, '.');
  if (!dot) return 0;

  char ext[6] = {};
  size_t n = strlen(dot);
  if (n >= sizeof(ext)) return 0;

  for (size_t i = 0; i < n; i++)
    ext[i] = (char)tolower((unsigned char)dot[i]);

  return !strcmp(ext, ".jpg") || !strcmp(ext, ".jpeg");
}

int ft_dxp_ext_match(const char *name)
{
  const char *dot = strrchr(name, '.');
  if (!dot) return 0;

  char ext[6] = {};
  size_t n = strlen(dot);
  if (n >= sizeof(ext)) return 0;

  for (size_t i = 0; i < n; i++)
    ext[i] = (char)tolower((unsigned char)dot[i]);

  return !strcmp(ext, ".dxp");
}

char *ft_jpg_strdup(const char *s)
{
  if (!s) return nullptr;

  size_t len = strlen(s) + 1;
  char *d = (char*)malloc(len);
  if (!d) return nullptr;

  memcpy(d, s, len);
  return d;
}

void ft_jpg_list_free(FT_JPG_LIST *lst)
{
  if (!lst) return;

  for (int i = 0; i < lst->count; i++)
    free(lst->items[i]);

  free(lst->items);
  lst->items = nullptr;
  lst->count = 0;
  lst->cap = 0;
}

int ft_jpg_list_add(FT_JPG_LIST *lst, const char *path)
{
  if (!lst || !path || !path[0]) return 0;

  if (lst->count >= lst->cap)
  {
    int new_cap = lst->cap ? lst->cap * 2 : 16;
    char **new_items = (char**)realloc(lst->items, (size_t)new_cap * sizeof(char*));
    if (!new_items) return 0;

    lst->items = new_items;
    lst->cap = new_cap;
  }

  lst->items[lst->count] = ft_jpg_strdup(path);
  if (!lst->items[lst->count]) return 0;

  lst->count++;
  return 1;
}

int ft_jpg_list_cmp(const void *a, const void *b)
{
  const char *pa = *(const char * const *)a;
  const char *pb = *(const char * const *)b;
  return strcmp(pa, pb);
}

int ft_jpg_join_rel(char *dst, size_t dst_size, const char *dir, const char *name)
{
  int n;

  if (!dst || !dst_size || !name || !name[0]) return 0;

  if (!dir || !dir[0] || !strcmp(dir, "/"))
    n = snprintf(dst, dst_size, "/%s", name);
  else if (dir[strlen(dir) - 1] == '/')
    n = snprintf(dst, dst_size, "%s%s", dir, name);
  else
    n = snprintf(dst, dst_size, "%s/%s", dir, name);

  return n >= 0 && (size_t)n < dst_size;
}

esp_err_t ft_jpg_scan_dir(const char *base, const char *rel, FT_JPG_LIST *lst, int depth)
{
  char full[FT_JPG_MAX_PATH];

  if (depth > FT_JPG_SCAN_MAX_DEPTH) return ESP_OK;

  if (!sd_fs_build_full_path(base, rel, full, sizeof(full)))
  {
    printf("E: path too long: %s\r\n", rel ? rel : "-");
    return ESP_ERR_INVALID_ARG;
  }

  DIR *d = opendir(full);
  if (!d)
  {
    int saved_errno = errno;
    printf("E: opendir('%s') failed, errno=%d\r\n", full, saved_errno);
    if (sd_has_retryable_errno(saved_errno)) return ESP_ERR_INVALID_STATE;
    return ESP_FAIL;
  }

  for (;;)
  {
    struct dirent *e = readdir(d);
    if (!e) break;

    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
      continue;

    char child_rel[FT_JPG_MAX_PATH];
    char child_full[FT_JPG_MAX_PATH];

    if (!ft_jpg_join_rel(child_rel, sizeof(child_rel), rel, e->d_name))
    {
      printf("W: skip long path: %s/%s\r\n", rel ? rel : "", e->d_name);
      continue;
    }

    if (!sd_fs_build_full_path(base, child_rel, child_full, sizeof(child_full)))
    {
      printf("W: skip long full path: %s\r\n", child_rel);
      continue;
    }

    struct stat st = {};
    if (stat(child_full, &st) != 0)
    {
      printf("W: stat('%s') failed, errno=%d\r\n", child_full, errno);
      continue;
    }

    if (S_ISDIR(st.st_mode))
    {
      esp_err_t err = ft_jpg_scan_dir(base, child_rel, lst, depth + 1);
      if (err != ESP_OK)
      {
        closedir(d);
        return err;
      }
      continue;
    }

    if (!S_ISREG(st.st_mode)) continue;
    if (!ft_jpg_ext_match(e->d_name)) continue;

    if (!ft_jpg_list_add(lst, child_rel))
    {
      closedir(d);
      return ESP_ERR_NO_MEM;
    }
  }

  closedir(d);
  return ESP_OK;
}

esp_err_t ft_jpg_collect(const char *path, FT_JPG_LIST *lst)
{
  const char *base = "/sd";
  char full[FT_JPG_MAX_PATH];
  sdmmc_card_t *card = nullptr;

  if (!path || !path[0]) return ESP_ERR_INVALID_ARG;
  if (!lst) return ESP_ERR_INVALID_ARG;

  esp_err_t err = sd_fs_mount(base, &card);
  if (err != ESP_OK && sd_error_needs_reinit(err))
  {
    printf("W: SD mount failed: %s, retry\r\n", esp_err_to_name(err));
    sd_deinit();
    err = sd_fs_mount(base, &card);
  }
  if (err != ESP_OK) return err;

  if (!sd_fs_build_full_path(base, path, full, sizeof(full)))
  {
    sd_fs_unmount(base, card);
    return ESP_ERR_INVALID_ARG;
  }

  struct stat st = {};
  if (stat(full, &st) != 0)
  {
    int saved_errno = errno;
    printf("E: stat('%s') failed, errno=%d\r\n", full, saved_errno);
    sd_fs_unmount(base, card);
    if (sd_has_retryable_errno(saved_errno)) return ESP_ERR_INVALID_STATE;
    return ESP_FAIL;
  }

  if (S_ISDIR(st.st_mode))
    err = ft_jpg_scan_dir(base, path, lst, 0);
  else if (S_ISREG(st.st_mode) && ft_jpg_ext_match(path))
    err = ft_jpg_list_add(lst, path) ? ESP_OK : ESP_ERR_NO_MEM;
  else
  {
    printf("E: no JPEG files at '%s'\r\n", path);
    err = ESP_ERR_NOT_FOUND;
  }

  sd_fs_unmount(base, card);

  if (err == ESP_OK && lst->count > 1)
    qsort(lst->items, (size_t)lst->count, sizeof(char*), ft_jpg_list_cmp);

  return err;
}


int ft_jpg_is_sof_marker(u8 marker)
{
  if (marker < 0xC0 || marker > 0xCF) return 0;
  if (marker == 0xC4 || marker == 0xC8 || marker == 0xCC) return 0;
  return 1;
}

int ft_jpg_read_sof(const u8 *jpg, size_t size, u8 *out_marker, u16 *out_w, u16 *out_h)
{
  if (!jpg || size < 4) return 0;
  if (jpg[0] != 0xFF || jpg[1] != 0xD8) return 0;

  size_t pos = 2;
  while (pos + 4 <= size)
  {
    if (jpg[pos] != 0xFF)
    {
      pos++;
      continue;
    }

    while (pos < size && jpg[pos] == 0xFF)
      pos++;

    if (pos >= size) return 0;

    u8 marker = jpg[pos++];
    if (marker == 0x00) continue;
    if (marker == 0xD9 || marker == 0xDA) return 0;
    if (marker >= 0xD0 && marker <= 0xD7) continue;
    if (pos + 2 > size) return 0;

    size_t seg_len = ((size_t)jpg[pos] << 8) | jpg[pos + 1];
    if (seg_len < 2 || pos + seg_len > size) return 0;

    if (ft_jpg_is_sof_marker(marker))
    {
      if (seg_len < 8) return 0;
      if (out_marker) *out_marker = marker;
      if (out_h) *out_h = ((u16)jpg[pos + 3] << 8) | jpg[pos + 4];
      if (out_w) *out_w = ((u16)jpg[pos + 5] << 8) | jpg[pos + 6];
      return 1;
    }

    pos += seg_len;
  }

  return 0;
}

void ft_jpg_copy_to_screen(u8 *screen, const u8 *img, u16 img_w, u16 img_h)
{
  if (!screen || !img) return;

  memset(screen, 0, FT_RGB888_640_480_SIZE);

  u16 copy_w = img_w > FT_RGB888_640_480_W ? FT_RGB888_640_480_W : img_w;
  u16 copy_h = img_h > FT_RGB888_640_480_H ? FT_RGB888_640_480_H : img_h;
  u16 dst_x = img_w < FT_RGB888_640_480_W ? (u16)((FT_RGB888_640_480_W - img_w) / 2U) : 0;
  u16 dst_y = img_h < FT_RGB888_640_480_H ? (u16)((FT_RGB888_640_480_H - img_h) / 2U) : 0;

  u8 *dst_r0 = screen;
  u8 *dst_g0 = screen + FT_RGB888_640_480_PLANE_SIZE;
  u8 *dst_b0 = screen + (FT_RGB888_640_480_PLANE_SIZE * 2UL);

  for (u16 y = 0; y < copy_h; y++)
  {
    const u8 *src = img + ((size_t)y * img_w * FT_RGB888_640_480_BPP);
    u8 *dst_r = dst_r0 + (((size_t)dst_y + y) * FT_RGB888_640_480_STRIDE) + dst_x;
    u8 *dst_g = dst_g0 + (((size_t)dst_y + y) * FT_RGB888_640_480_STRIDE) + dst_x;
    u8 *dst_b = dst_b0 + (((size_t)dst_y + y) * FT_RGB888_640_480_STRIDE) + dst_x;

    for (u16 x = 0; x < copy_w; x++)
    {
      const u8 *px = src + ((size_t)x * FT_RGB888_640_480_BPP);
      dst_r[x] = px[0];
      dst_g[x] = px[1];
      dst_b[x] = px[2];
    }
  }
}

esp_err_t ft_jpg_read_whole_file(const char *path, u8 **out_data, size_t *out_size)
{
  FILE *fp;
  long file_size;
  u8 *data;
  size_t got;
  size_t sd_size = 0;
  esp_err_t err;

  if (!path || !out_data || !out_size) return ESP_ERR_INVALID_ARG;

  *out_data = nullptr;
  *out_size = 0;

  fp = fopen(path, "rb");
  if (fp)
  {
    if (fseek(fp, 0, SEEK_END) != 0)
    {
      fclose(fp);
      return ESP_FAIL;
    }

    file_size = ftell(fp);
    if (file_size <= 0 || file_size > INT_MAX)
    {
      fclose(fp);
      return ESP_ERR_INVALID_SIZE;
    }

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
      fclose(fp);
      return ESP_FAIL;
    }

    data = (u8*)heap_caps_malloc((size_t)file_size, MALLOC_CAP_SPIRAM);
    if (!data)
    {
      fclose(fp);
      return ESP_ERR_NO_MEM;
    }

    got = fread(data, 1, (size_t)file_size, fp);
    if (got != (size_t)file_size)
    {
      free(data);
      fclose(fp);
      return ESP_FAIL;
    }

    fclose(fp);
    *out_data = data;
    *out_size = (size_t)file_size;
    return ESP_OK;
  }

  err = sd_fs_read_file("/sd", path, NULL, 0, &sd_size);
  if (err != ESP_OK) return err;
  if (sd_size == 0 || sd_size > INT_MAX) return ESP_ERR_INVALID_SIZE;

  data = (u8*)heap_caps_malloc(sd_size, MALLOC_CAP_SPIRAM);
  if (!data) return ESP_ERR_NO_MEM;

  err = sd_fs_read_file("/sd", path, data, sd_size, NULL);
  if (err != ESP_OK)
  {
    free(data);
    return err;
  }

  *out_data = data;
  *out_size = sd_size;
  return ESP_OK;
}

esp_err_t ft_jpg_decode_to_screen(const char *path, u8 *screen, u16 *out_w, u16 *out_h, bool quiet)
{
  size_t jpg_size = 0;
  u8 *jpg = nullptr;
  u8 *decoded = nullptr;
  jpeg_dec_handle_t dec = NULL;
  jpeg_dec_io_t *io = NULL;
  jpeg_dec_header_info_t *info = NULL;
  jpeg_dec_config_t cfg = DEFAULT_JPEG_DEC_CONFIG();
  jpeg_error_t jerr = JPEG_ERR_OK;
  int output_len = 0;
  u8 sof_marker = 0;
  u16 sof_w = 0;
  u16 sof_h = 0;

  if (!path || !screen) return ESP_ERR_INVALID_ARG;

  esp_err_t err = ft_jpg_read_whole_file(path, &jpg, &jpg_size);
  if (err != ESP_OK) return err;

  if (ft_jpg_read_sof(jpg, jpg_size, &sof_marker, &sof_w, &sof_h))
  {
    if (sof_marker == 0xC2)
    {
      if (!quiet)
      {
        printf("E: progressive JPEG is not supported by esp_new_jpeg: '%s' (%ux%u, SOF2)\r\n",
          path, (unsigned)sof_w, (unsigned)sof_h);
      }
      free(jpg);
      return ESP_ERR_NOT_SUPPORTED;
    }

    if (sof_marker != 0xC0)
    {
      if (!quiet)
      {
        printf("E: unsupported JPEG SOF marker 0x%02X: '%s' (%ux%u)\r\n",
          (unsigned)sof_marker, path, (unsigned)sof_w, (unsigned)sof_h);
      }
      free(jpg);
      return ESP_ERR_NOT_SUPPORTED;
    }
  }

  cfg.output_type = JPEG_PIXEL_FORMAT_RGB888;
  cfg.rotate = JPEG_ROTATE_0D;

  jerr = jpeg_dec_open(&cfg, &dec);
  if (jerr != JPEG_ERR_OK)
  {
    if (!quiet)
      printf("E: jpeg_dec_open failed for '%s': %d\r\n", path, (int)jerr);
    free(jpg);
    return jerr == JPEG_ERR_NO_MEM ? ESP_ERR_NO_MEM : ESP_FAIL;
  }

  io = (jpeg_dec_io_t*)calloc(1, sizeof(jpeg_dec_io_t));
  info = (jpeg_dec_header_info_t*)calloc(1, sizeof(jpeg_dec_header_info_t));
  if (!io || !info)
  {
    if (!quiet)
      printf("E: JPEG decoder helper alloc failed\r\n");
    err = ESP_ERR_NO_MEM;
    goto done;
  }

  io->inbuf = jpg;
  io->inbuf_len = (int)jpg_size;

  jerr = jpeg_dec_parse_header(dec, io, info);
  if (jerr != JPEG_ERR_OK)
  {
    if (!quiet)
      printf("E: JPEG header parse failed for '%s': %d\r\n", path, (int)jerr);
    err = jerr == JPEG_ERR_NO_MEM ? ESP_ERR_NO_MEM : ESP_FAIL;
    goto done;
  }

  if (info->width == 0 || info->height == 0)
  {
    if (!quiet)
    {
      printf("E: bad JPEG info for '%s': %ux%u\r\n",
        path, (unsigned)info->width, (unsigned)info->height);
    }
    err = ESP_FAIL;
    goto done;
  }

  jerr = jpeg_dec_get_outbuf_len(dec, &output_len);
  if (jerr != JPEG_ERR_OK || output_len <= 0)
  {
    if (!quiet)
    {
      printf("E: JPEG outbuf len failed for '%s': %d len=%d\r\n",
        path, (int)jerr, output_len);
    }
    err = jerr == JPEG_ERR_NO_MEM ? ESP_ERR_NO_MEM : ESP_FAIL;
    goto done;
  }

  decoded = (u8*)jpeg_calloc_align((size_t)output_len, 16);
  if (!decoded)
  {
    if (!quiet)
    {
      printf("E: JPEG output alloc failed: %ux%u, size=%d\r\n",
        (unsigned)info->width, (unsigned)info->height, output_len);
    }
    err = ESP_ERR_NO_MEM;
    goto done;
  }

  io->outbuf = decoded;

  jerr = jpeg_dec_process(dec, io);
  if (jerr != JPEG_ERR_OK)
  {
    if (!quiet)
      printf("E: JPEG decode failed for '%s': %d\r\n", path, (int)jerr);
    err = jerr == JPEG_ERR_NO_MEM ? ESP_ERR_NO_MEM : ESP_FAIL;
    goto done;
  }

  ft_jpg_copy_to_screen(screen, decoded, info->width, info->height);

  if (out_w) *out_w = info->width;
  if (out_h) *out_h = info->height;

  err = ESP_OK;

done:
  if (dec) jpeg_dec_close(dec);
  if (decoded) jpeg_free_align(decoded);
  if (info) free(info);
  if (io) free(io);
  if (jpg) free(jpg);
  return err;
}

u16 ft_dxp_read_le16(const u8 *p)
{
  return (u16)p[0] | ((u16)p[1] << 8);
}

esp_err_t ft_dxp_parse_header(const u8 *data, size_t size, FT_DXP_INFO *info, bool quiet)
{
  u32 color_size;
  u32 mask_stride;
  u32 mask_size;

  if (!data || !info) return ESP_ERR_INVALID_ARG;
  if (size < FT_DXP_HEADER_SIZE) return ESP_ERR_INVALID_SIZE;
  if (data[0] != 'D' || data[1] != 'X' || data[2] != 'P') return ESP_ERR_INVALID_RESPONSE;

  memset(info, 0, sizeof(*info));
  info->type = data[3];
  info->w = ft_dxp_read_le16(&data[4]);
  info->h = ft_dxp_read_le16(&data[6]);

  if (info->w == 0 || info->h == 0) return ESP_ERR_INVALID_SIZE;
  if ((info->w & 3) || (info->h & 3)) return ESP_ERR_INVALID_SIZE;

  switch (info->type)
  {
    case FT_DXP_TYPE_RAW_L2:
    case FT_DXP_TYPE_ZLIB_L2:
      info->mask_format = FT_L2;
      mask_stride = (u32)info->w >> 2;
      break;

    case FT_DXP_TYPE_ZLIB_L4:
    case FT_DXP_TYPE_RAW_L4:
      info->mask_format = FT_L4;
      mask_stride = (u32)info->w >> 1;
      break;

    default:
      if (!quiet)
        printf("E: unsupported DXP type %u\r\n", (unsigned)info->type);
      return ESP_ERR_NOT_SUPPORTED;
  }

  color_size = ((u32)info->w >> 2) * ((u32)info->h >> 2) * 2UL;
  mask_size = mask_stride * (u32)info->h;
  info->raw_size = color_size + color_size + mask_size;

  if (info->raw_size == 0 || info->raw_size > FT_DXP_RAM_G_SIZE) return ESP_ERR_INVALID_SIZE;
  return ESP_OK;
}

bool ft_dxp_type_is_raw(u8 type)
{
  return type == FT_DXP_TYPE_RAW_L2 || type == FT_DXP_TYPE_RAW_L4;
}

esp_err_t ft_dxp_inflate_zlib(const u8 *src, size_t src_size, u8 *dst, size_t dst_size, bool quiet)
{
  tinfl_decompressor *decomp;
  size_t inbytes;
  size_t outbytes;
  tinfl_status decomp_status;
  esp_err_t err = ESP_OK;

  if (!src || !dst) return ESP_ERR_INVALID_ARG;
  if (!src_size || !dst_size) return ESP_ERR_INVALID_SIZE;

  decomp = (tinfl_decompressor*)heap_caps_malloc(
    sizeof(tinfl_decompressor),
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!decomp) return ESP_ERR_NO_MEM;

  inbytes = src_size;
  outbytes = dst_size;

  tinfl_init(decomp);

  decomp_status = tinfl_decompress(
    decomp,
    src,
    &inbytes,
    dst,
    dst,
    &outbytes,
    TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

#ifdef VERBOSE
  printf("DXP inflate status=%d, inbytes=%u, outbytes=%u\r\n",
    (int)decomp_status,
    (unsigned)inbytes,
    (unsigned)outbytes);
#endif

  if (decomp_status != TINFL_STATUS_DONE || outbytes != dst_size)
  {
    if (!quiet)
    {
      printf("E: DXP inflate failed: status=%d in=%u/%u out=%u/%u\r\n",
        (int)decomp_status,
        (unsigned)inbytes,
        (unsigned)src_size,
        (unsigned)outbytes,
        (unsigned)dst_size);
    }
    err = ESP_FAIL;
  }

  free(decomp);
  return err;
}

esp_err_t ft_dxp_prepare_raw(const u8 *data, size_t size, const FT_DXP_INFO *info,
  const u8 **out_raw, u8 **out_alloc, bool quiet)
{
  const u8 *payload;
  size_t payload_size;
  esp_err_t err;

  if (!data || !info || !out_raw || !out_alloc) return ESP_ERR_INVALID_ARG;
  if (size < FT_DXP_HEADER_SIZE) return ESP_ERR_INVALID_SIZE;

  *out_raw = nullptr;
  *out_alloc = nullptr;
  payload = data + FT_DXP_HEADER_SIZE;
  payload_size = size - FT_DXP_HEADER_SIZE;

  if (ft_dxp_type_is_raw(info->type))
  {
    if (payload_size < info->raw_size) return ESP_ERR_INVALID_SIZE;
    *out_raw = payload;
    return ESP_OK;
  }

  u8 *raw = (u8*)heap_caps_malloc(info->raw_size, MALLOC_CAP_SPIRAM);
  if (!raw) return ESP_ERR_NO_MEM;

  err = ft_dxp_inflate_zlib(payload, payload_size, raw, info->raw_size, quiet);
  if (err != ESP_OK)
  {
    free(raw);
    return err;
  }

  *out_raw = raw;
  *out_alloc = raw;
  return ESP_OK;
}

void ft_dxp_dl(const FT_DXP_INFO *info, u16 x, u16 y)
{
  u32 mask_addr;

  if (!info) return;

  mask_addr = FT_RAM_G + ((u32)info->w * ((u32)info->h >> 2));

  ft_BitmapHandle(0);
  ft_SetBitmap(mask_addr, info->mask_format, info->w, info->h);
  ft_BitmapHandle(1);
  ft_SetBitmap(FT_RAM_G, FT_RGB565, info->w >> 2, info->h >> 2);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, info->w, info->h);

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
}

esp_err_t ft_dxp_show_mode(const FT_DXP_INFO *info)
{
  esp_err_t err;
  u16 screen_w = 1024;
  u16 screen_h = 768;
  u16 x = 0;
  u16 y = 0;

  if (!info) return ESP_ERR_INVALID_ARG;

  if (ft_current_mode >= 0 && ft_current_mode < FT_MODE_MAX)
  {
    screen_w = ft_modes[ft_current_mode].h_visible;
    screen_h = ft_modes[ft_current_mode].v_visible;
  }

  if (screen_w > info->w)
    x = (u16)((screen_w - info->w) >> 1);

  if (screen_h > info->h)
    y = (u16)((screen_h - info->h) >> 1);

  ft_ccmd_start(cmdl);
  ft_Dlstart();
  ft_ClearColorRGB(0, 0, 0);
  ft_ClearColorA(255);
  ft_Clear(1, 1, 1);

  ft_dxp_dl(info, x, y);

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ft_wait_swap(1000);
}

esp_err_t ft_dxp_show_file_mode(const char *path, bool quiet)
{
  u8 *data = nullptr;
  size_t size = 0;
  const u8 *raw = nullptr;
  u8 *raw_alloc = nullptr;
  FT_DXP_INFO info;
  esp_err_t err;
  esp_err_t err2;
  bool keep_session = false;

  if (!path) return ESP_ERR_INVALID_ARG;

  int64_t t0 = esp_timer_get_time();
  err = ft_jpg_read_whole_file(path, &data, &size);
  if (err != ESP_OK) return err;

  err = ft_dxp_parse_header(data, size, &info, quiet);
  if (err != ESP_OK) goto done_no_session;

  err = ft_dxp_prepare_raw(data, size, &info, &raw, &raw_alloc, quiet);
  if (err != ESP_OK) goto done_no_session;

  keep_session = ft_text_attr_mode_open;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    if (!quiet)
      printf("FT open failed: %d\r\n", (int)err);
    goto done_no_session;
  }

  err = ft_dxp_show_mode(&info);
  if (err == ESP_OK)
  {
    err = ft_write(raw, FT_RAM_G, info.raw_size);
    if (err == ESP_OK) ft_text_attr_mode_palette_dirty = true;
  }

  if (!keep_session)
  {
    err2 = ft_close_session();
    if (err == ESP_OK) err = err2;
  }

  if (err == ESP_OK && !quiet)
  {
    int64_t t1 = esp_timer_get_time();
    printf("DXP: %s  %ux%u type=%u raw=%lu load=%lldus\r\n",
      path, (unsigned)info.w, (unsigned)info.h, (unsigned)info.type,
      (unsigned long)info.raw_size, (long long)(t1 - t0));
  }

done_no_session:
  if (raw_alloc) free(raw_alloc);
  if (data) free(data);
  return err;
}

esp_err_t ft_dxp_show_file(const char *path)
{
  return ft_dxp_show_file_mode(path, false);
}

esp_err_t ft_jpg_show_file_mode(const char *path, bool stretch, bool quiet)
{
  esp_err_t err;
  esp_err_t err2;
  u16 img_w = 0;
  u16 img_h = 0;

  u8 *screen = (u8*)heap_caps_malloc(FT_RGB888_640_480_SIZE, MALLOC_CAP_SPIRAM);
  if (!screen)
  {
    if (!quiet)
      printf("E: RGB888 screen alloc failed, size=%lu\r\n", (unsigned long)FT_RGB888_640_480_SIZE);
    return ESP_ERR_NO_MEM;
  }

  int64_t t0 = esp_timer_get_time();
  err = ft_jpg_decode_to_screen(path, screen, &img_w, &img_h, quiet);
  int64_t t1 = esp_timer_get_time();
  if (err != ESP_OK)
  {
    free(screen);
    return err;
  }

  bool keep_session = ft_text_attr_mode_open;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    if (!quiet)
      printf("FT open failed: %d\r\n", (int)err);
    free(screen);
    return err;
  }

  err = ft_rgb888_640x480_show_mode(FT_RGB888_640_480_ADDR, stretch);
  if (err == ESP_OK)
  {
    err = ft_write(screen, FT_RGB888_640_480_ADDR, FT_RGB888_640_480_SIZE);
    if (err == ESP_OK) ft_text_attr_mode_palette_dirty = true;
  }

  if (!keep_session)
  {
    err2 = ft_close_session();
    if (err == ESP_OK)
      err = err2;
  }

  free(screen);

  if (err != ESP_OK)
    return err;

  if (!quiet)
  {
    printf("JPEG: %s  %ux%u -> 640x480  decode=%lldus\r\n",
      path, (unsigned)img_w, (unsigned)img_h, (long long)(t1 - t0));
  }

  return ESP_OK;
}

esp_err_t ft_jpg_show_file(const char *path)
{
  return ft_jpg_show_file_mode(path, false, false);
}

int ft_jpg_wait_key()
{
  u8 c;

  if (uart_read_bytes(UART_NUM_0, &c, 1, portMAX_DELAY) <= 0)
    return FT_JPG_KEY_EXIT;

  if (c != 0x1B)
    return FT_JPG_KEY_EXIT;

  u8 seq[2] = {};
  if (uart_read_bytes(UART_NUM_0, &seq[0], 1, pdMS_TO_TICKS(50)) <= 0)
    return FT_JPG_KEY_EXIT;

  if (seq[0] != '[')
    return FT_JPG_KEY_EXIT;

  if (uart_read_bytes(UART_NUM_0, &seq[1], 1, pdMS_TO_TICKS(50)) <= 0)
    return FT_JPG_KEY_EXIT;

  if (seq[1] == 'C') return FT_JPG_KEY_NEXT;
  if (seq[1] == 'D') return FT_JPG_KEY_PREV;

  return FT_JPG_KEY_EXIT;
}

int ft_dxp_cmd(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Usage:\r\n");
    printf("  ft dxp <path>\r\n");
    return 1;
  }

  esp_err_t err = ft_dxp_show_file(argv[2]);
  if (err != ESP_OK)
  {
    printf("E: DXP show failed: %s (0x%x)\r\n", esp_err_to_name(err), (unsigned int)err);
    return 1;
  }

  return 0;
}

int ft_jpg_cmd(int argc, char **argv)
{
  if (argc != 3)
  {
    printf("Usage:\r\n");
    printf("  ft jpg <path>\r\n");
    return 1;
  }

  FT_JPG_LIST lst = {};
  esp_err_t err = ft_jpg_collect(argv[2], &lst);
  if (err != ESP_OK)
  {
    ft_jpg_list_free(&lst);
    return 1;
  }

  if (lst.count <= 0)
  {
    printf("No JPEG files found: %s\r\n", argv[2]);
    ft_jpg_list_free(&lst);
    return 1;
  }

  printf("JPEG files: %d\r\n", lst.count);
  printf("Controls: Right=next, Left=prev, other key=exit\r\n");

  int idx = 0;
  for (;;)
  {
    printf("[%d/%d] %s\r\n", idx + 1, lst.count, lst.items[idx]);

    err = ft_jpg_show_file(lst.items[idx]);
    if (err != ESP_OK)
      printf("E: show failed: %s\r\n", esp_err_to_name(err));

    int key = ft_jpg_wait_key();
    if (key == FT_JPG_KEY_NEXT)
    {
      idx++;
      if (idx >= lst.count) idx = 0;
      continue;
    }

    if (key == FT_JPG_KEY_PREV)
    {
      idx--;
      if (idx < 0) idx = lst.count - 1;
      continue;
    }

    break;
  }

  ft_jpg_list_free(&lst);
  return 0;
}

void ft_rgb888_640x480_dl(u32 rgb_addr, u16 screen_x, u16 screen_y, u16 draw_w, u16 draw_h, u8 filter)
{
  i32 scale_x;
  i32 scale_y;

  if (draw_w == 0) draw_w = FT_RGB888_640_480_W;
  if (draw_h == 0) draw_h = FT_RGB888_640_480_H;

  scale_x = (i32)(((u32)FT_RGB888_640_480_W * 256U + ((u32)draw_w / 2U)) / draw_w);
  scale_y = (i32)(((u32)FT_RGB888_640_480_H * 256U + ((u32)draw_h / 2U)) / draw_h);

  ft_SaveContext();

  ft_BitmapHandle(0);
  ft_BitmapLayout(FT_L8, (u16)FT_RGB888_640_480_STRIDE, FT_RGB888_640_480_H);
  ft_BitmapSize(filter, FT_BORDER, FT_BORDER, draw_w, draw_h);

  ft_BitmapTransformA(scale_x);
  ft_BitmapTransformB(0);
  ft_BitmapTransformC(0);
  ft_BitmapTransformD(0);
  ft_BitmapTransformE(scale_y);
  ft_BitmapTransformF(0);

  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);
  ft_ColorA(255);
  ft_ColorMask(1, 1, 1, 0);
  ft_Begin(FT_BITMAPS);

  ft_ColorRGB(255, 0, 0);
  ft_BitmapSource(rgb_addr);
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_ColorRGB(0, 255, 0);
  ft_BitmapSource(rgb_addr + FT_RGB888_640_480_PLANE_SIZE);
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_ColorRGB(0, 0, 255);
  ft_BitmapSource(rgb_addr + (FT_RGB888_640_480_PLANE_SIZE * 2UL));
  ft_Vertex2ii(screen_x, screen_y, 0, 0);

  ft_End();

  ft_RestoreContext();
}

esp_err_t ft_rgb888_640x480_load_mode(u32 rgb_addr, bool stretch, bool swap)
{
  esp_err_t err;
  u16 screen_x = 0;
  u16 screen_y = 0;
  u16 screen_w = FT_RGB888_640_480_W;
  u16 screen_h = FT_RGB888_640_480_H;
  u16 draw_w = FT_RGB888_640_480_W;
  u16 draw_h = FT_RGB888_640_480_H;
  u8 filter = FT_NEAREST;

  if (ft_current_mode >= 0 && ft_current_mode < FT_MODE_MAX)
  {
    screen_w = ft_modes[ft_current_mode].h_visible;
    screen_h = ft_modes[ft_current_mode].v_visible;
  }

  if (stretch)
  {
    draw_w = screen_w;
    draw_h = screen_h;
    filter = FT_BILINEAR;
  }
  else
  {
    if (screen_w > FT_RGB888_640_480_W)
      screen_x = (u16)((screen_w - FT_RGB888_640_480_W) / 2);

    if (screen_h > FT_RGB888_640_480_H)
      screen_y = (u16)((screen_h - FT_RGB888_640_480_H) / 2);
  }

  ft_ccmd_start(cmdl);

  ft_Dlstart();
  ft_ClearColorRGB(0, 0, 0);
  ft_ClearColorA(255);
  ft_Clear(1, 1, 1);

  ft_rgb888_640x480_dl(rgb_addr, screen_x, screen_y, draw_w, draw_h, filter);

  ft_Display();
  if (swap) ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  if (!swap) return ESP_OK;

  return ft_wait_swap(1000);
}

esp_err_t ft_rgb888_640x480_show_mode(u32 rgb_addr, bool stretch)
{
  return ft_rgb888_640x480_load_mode(rgb_addr, stretch, true);
}

esp_err_t ft_rgb888_640x480_show(u32 rgb_addr)
{
  return ft_rgb888_640x480_show_mode(rgb_addr, false);
}

esp_err_t ft_rgb888_640x480_show_current(bool stretch)
{
  esp_err_t err;
  esp_err_t err2;
  bool keep_session = ft_text_attr_mode_open;

  err = ft_open_session();
  if (err != ESP_OK) return err;

  err = ft_rgb888_640x480_show_mode(FT_RGB888_640_480_ADDR, stretch);

  if (!keep_session)
  {
    err2 = ft_close_session();
    if (err == ESP_OK) err = err2;
  }

  return err;
}

int ft_demo_rgb888_cmd()
{
  esp_err_t err;
  esp_err_t err2;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_rgb888_640x480_show_mode(FT_RGB888_640_480_ADDR, false);

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT demo RGB888 failed: %d\r\n", (int)err);
    return 1;
  }

  printf("FT RGB888 640x480 shown from 0x%06lX, size=%lu\r\n",
         (unsigned long)FT_RGB888_640_480_ADDR,
         (unsigned long)FT_RGB888_640_480_SIZE);

  return 0;
}

int ft_demo_attr_mode_cmd()
{
  esp_err_t err;
  esp_err_t err2;
  u8 *chars = nullptr;
  u8 *attrs = nullptr;
  u8 *gfx = nullptr;
  u8 *color = nullptr;
  u32 text_cells = (u32)FT_DEMO_TEXT_COLS * (u32)FT_DEMO_TEXT_ROWS;
  u32 gfx_size = (u32)FT_DEMO_TEXT_COLS * 256U;
  u32 color_size = (u32)FT_DEMO_TEXT_ROWS * 256U;

  chars = (u8*)heap_caps_malloc(text_cells, MALLOC_CAP_SPIRAM);
  attrs = (u8*)heap_caps_malloc(text_cells, MALLOC_CAP_SPIRAM);
  gfx = (u8*)heap_caps_malloc(gfx_size, MALLOC_CAP_SPIRAM);
  color = (u8*)heap_caps_malloc(color_size, MALLOC_CAP_SPIRAM);

  if (!chars || !attrs || !gfx || !color)
  {
    printf("FT demo_attr_mode alloc failed: chars=%u attrs=%u gfx=%lu color=%lu\r\n",
      (unsigned int)text_cells,
      (unsigned int)text_cells,
      (unsigned long)gfx_size,
      (unsigned long)color_size);
    if (chars) free(chars);
    if (attrs) free(attrs);
    if (gfx) free(gfx);
    if (color) free(color);
    return 1;
  }

  ft_text_set_addrs(chars, attrs);
  ft_demo_text_fill("Attribute mode text demo", "80x30 chars -> FT812 640x480");
  ft_text_to_attr_render(gfx, chars, color, attrs);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    free(chars);
    free(attrs);
    free(gfx);
    free(color);
    return 1;
  }

  if (err == ESP_OK)
    err = ft_write(gfx, FT_DEMO_PIX_ADDR(0), gfx_size);

  if (err == ESP_OK)
    err = ft_write(color, FT_DEMO_ATTR_ADDR, color_size);

  if (err == ESP_OK)
    err = ft_text_palette_init();

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);
    err = ft_attr_show();
  }

  if (err == ESP_OK)
    err = ft_wait_swap(1000);

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  free(chars);
  free(attrs);
  free(gfx);
  free(color);

  if (err != ESP_OK)
  {
    printf("FT demo_attr_mode failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

int ft_demo_4bpp_mode_cmd()
{
  esp_err_t err;
  esp_err_t err2;
  u8 *chars = nullptr;
  u8 *attrs = nullptr;
  u8 *bitmap = nullptr;

  u32 text_cells = (u32)FT_DEMO_TEXT_COLS * (u32)FT_DEMO_TEXT_ROWS;
  u32 bitmap_size = ((u32)FT_DEMO_TEXT_COLS * 4U) * ((u32)FT_DEMO_TEXT_ROWS * 8U);

  chars = (u8*)heap_caps_malloc(text_cells, MALLOC_CAP_SPIRAM);
  attrs = (u8*)heap_caps_malloc(text_cells, MALLOC_CAP_SPIRAM);
  bitmap = (u8*)heap_caps_malloc(bitmap_size, MALLOC_CAP_SPIRAM);

  if (!chars || !attrs || !bitmap)
  {
    printf("FT demo_4bpp_mode alloc failed: chars=%u attrs=%u bitmap=%lu\r\n",
      (unsigned int)text_cells,
      (unsigned int)text_cells,
      (unsigned long)bitmap_size);
    if (chars) free(chars);
    if (attrs) free(attrs);
    if (bitmap) free(bitmap);
    return 1;
  }

  ft_text_set_addrs(chars, attrs);
  ft_demo_text_fill("4bpp paletted text demo", "80x30 chars -> FT812 640x480");
  ft_text_to_4bpp_render(bitmap, chars, attrs);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    free(chars);
    free(attrs);
    free(bitmap);
    return 1;
  }

  if (err == ESP_OK)
    err = ft_write(bitmap, FT_DEMO_PAL4_PIX_ADDR, bitmap_size);

  if (err == ESP_OK)
    err = ft_text_palette_init();

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);
    err = ft_4bpp_show();
  }

  if (err == ESP_OK)
    err = ft_wait_swap(1000);

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  free(chars);
  free(attrs);
  free(bitmap);

  if (err != ESP_OK)
  {
    printf("FT demo_4bpp_mode failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

u8 ft_zx_color_from_attr(u8 attr, u8 paper)
{
  u8 color;
  u8 bright;

  attr &= 0x7FU;
  bright = (attr & 0x40U) ? 8U : 0U;

  if (paper)
    color = (u8)((attr >> 3) & 7U);
  else
    color = (u8)(attr & 7U);

  if (color)
    color |= bright;

  return color;
}

esp_err_t ft_zx_palette_init()
{
  esp_err_t err;
  u16 pal_paper[256];
  u16 pal_ink[256];

  for (u32 attr = 0; attr < 256; attr++)
  {
    pal_paper[attr] = ft_attr_to_rgb565(ft_zx_color_from_attr((u8)attr, 1));
    pal_ink[attr] = ft_attr_to_rgb565(ft_zx_color_from_attr((u8)attr, 0));
  }

  err = ft_write(pal_paper, FT_DEMO_ZX_PAL_PAPER_ADDR, sizeof(pal_paper));
  if (err != ESP_OK) return err;

  err = ft_write(pal_ink, FT_DEMO_ZX_PAL_INK_ADDR, sizeof(pal_ink));
  if (err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t ft_zx_show(u8 scale = 2)
{
  esp_err_t err;
  u16 screen_x = FT_DEMO_ZX_SCREEN_X;
  u16 screen_y = FT_DEMO_ZX_SCREEN_Y;

  ft_ccmd_start(cmdl);

  ft_Dlstart();

  ft_ClearColorRGB(0, 0, 32);
  ft_ClearColorA(255);
  ft_ClearStencil(0);
  ft_Clear(1, 1, 1);

  for (int h = 0; h < 8; h++)
  {
    ft_BitmapHandle(h);
    ft_BitmapSource(FT_DEMO_ZX_PIX_ADDR + h * 32);
    ft_BitmapLayout(FT_L1, 256, 8);
    ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 256 * scale, 8 * scale);
  }

  ft_BitmapHandle(8);
  ft_BitmapSource(FT_DEMO_ZX_ATTR_ADDR);
  ft_BitmapLayout(FT_PALETTED565, 32, 24);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, 256 * scale, 192 * scale);

  ft_BitmapTransformB(0);
  ft_BitmapTransformC(0);
  ft_BitmapTransformD(0);
  ft_BitmapTransformF(0);

  ft_BitmapTransformA(256 / scale);
  ft_BitmapTransformE(256 / scale);

  ft_ColorMask(0, 0, 0, 0);
  ft_StencilMask(255);
  ft_AlphaFunc(FT_GREATER, 0);
  ft_StencilFunc(FT_ALWAYS, 1, 255);
  ft_StencilOp(FT_KEEP, FT_REPLACE);

  ft_Begin(FT_BITMAPS);

  ft_VertexTranslateX((i32)screen_x * 16);
  ft_VertexTranslateY((i32)screen_y * 16);

  for (u8 bank = 0; bank < 3; bank++)
    for (u8 row = 0; row < 8; row++)
      ft_Vertex2ii(0, (bank * 64 + row * 8) * scale, row, bank);

  ft_ColorMask(1, 1, 1, 0);
  ft_StencilMask(0);
  ft_AlphaFunc(FT_ALWAYS, 0);
  ft_BlendFunc(FT_ONE, FT_ZERO);

  ft_BitmapTransformA(32 / scale);
  ft_BitmapTransformE(32 / scale);

  ft_StencilFunc(FT_EQUAL, 0, 255);
  ft_PaletteSource(FT_DEMO_ZX_PAL_PAPER_ADDR);
  ft_Vertex2ii(0, 0, 8, 0);

  ft_StencilFunc(FT_EQUAL, 1, 255);
  ft_PaletteSource(FT_DEMO_ZX_PAL_INK_ADDR);
  ft_Vertex2ii(0, 0, 8, 0);

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

int ft_demo_zx_mode_cmd()
{
  esp_err_t err;
  esp_err_t err2;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_write(zx_screen, FT_DEMO_ZX_PIX_ADDR, 6912);

  if (err == ESP_OK)
    err = ft_zx_palette_init();

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);
    err = ft_zx_show();
  }

  if (err == ESP_OK)
    err = ft_wait_swap(1000);

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT demo_zx_mode failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

// -------- Display list decoder ---------

#define FT_DL_READ_SIZE FT_DL_SIZE
#define FT_DL_DEFAULT_VERTEX_FORMAT 4
#define FT_DL_CONTEXT_STACK_SIZE 4

u32 ft_rd32le(const u8 *p)
{
  return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

i32 ft_dl_sx(u32 v, u8 bits)
{
  u32 m = 1UL << (bits - 1);
  return (i32)((v ^ m) - m);
}


typedef struct
{
  u8 vertex_format;
  u8 stack[FT_DL_CONTEXT_STACK_SIZE];
  u8 stack_depth;
} FT_DL_DECODE_CONTEXT;

void ft_dl_context_init(FT_DL_DECODE_CONTEXT *ctx)
{
  ctx->vertex_format = FT_DL_DEFAULT_VERTEX_FORMAT;
  ctx->stack_depth = 0;
}

void ft_dl_context_save(FT_DL_DECODE_CONTEXT *ctx)
{
  if (ctx->stack_depth < FT_DL_CONTEXT_STACK_SIZE)
  {
    ctx->stack[ctx->stack_depth++] = ctx->vertex_format;
    return;
  }

  for (u32 i = 1; i < FT_DL_CONTEXT_STACK_SIZE; i++)
    ctx->stack[i - 1] = ctx->stack[i];

  ctx->stack[FT_DL_CONTEXT_STACK_SIZE - 1] = ctx->vertex_format;
}

void ft_dl_context_restore(FT_DL_DECODE_CONTEXT *ctx)
{
  if (ctx->stack_depth)
  {
    ctx->vertex_format = ctx->stack[--ctx->stack_depth];
    return;
  }

  ctx->vertex_format = FT_DL_DEFAULT_VERTEX_FORMAT;
}

void ft_dl_print_fixed(i32 v, u8 frac)
{
  u32 uv;
  u32 ip;
  u32 rem;
  u32 den;
  u32 dec;
  char dec_txt[16];
  int n;

  if (frac > 4)
    frac = 4;

  if (v < 0)
  {
    printf("-");
    uv = (u32)(-(v + 1)) + 1UL;
  }
  else
  {
    uv = (u32)v;
  }

  den = 1UL << frac;
  ip = uv >> frac;
  rem = uv & (den - 1);

  printf("%lu", (unsigned long)ip);

  if (!frac || !rem)
    return;

  dec = (rem * 10000UL) / den;
  snprintf(dec_txt, sizeof(dec_txt), "%04lu", (unsigned long)dec);

  n = 3;
  while (n > 0 && dec_txt[n] == '0')
    dec_txt[n--] = 0;

  printf(".%s", dec_txt);
}

void ft_dl_print_fixed_cmd_1(u32 line, const char *name, i32 v, u8 frac)
{
  printf("%04lX %s(", (unsigned long)line, name);
  ft_dl_print_fixed(v, frac);
  printf(")\r\n");
}

const char *ft_dl_format_name(u32 v)
{
  switch (v)
  {
    case FT_ARGB1555:     return "ARGB1555";
    case FT_L1:           return "L1";
    case FT_L4:           return "L4";
    case FT_L8:           return "L8";
    case FT_RGB332:       return "RGB332";
    case FT_ARGB2:        return "ARGB2";
    case FT_ARGB4:        return "ARGB4";
    case FT_RGB565:       return "RGB565";
    case FT_TEXT8X8:      return "TEXT8X8";
    case FT_TEXTVGA:      return "TEXTVGA";
    case FT_BARGRAPH:     return "BARGRAPH";
    case FT_PALETTED565:  return "PALETTED565";
    case FT_PALETTED4444: return "PALETTED4444";
    case FT_PALETTED8:    return "PALETTED8";
    case FT_L2:           return "L2";
    default:              return "?";
  }
}

const char *ft_dl_blend_name(u32 v)
{
  switch (v)
  {
    case FT_ZERO:                return "ZERO";
    case FT_ONE:                 return "ONE";
    case FT_SRC_ALPHA:           return "SRC_ALPHA";
    case FT_DST_ALPHA:           return "DST_ALPHA";
    case FT_ONE_MINUS_SRC_ALPHA: return "ONE_MINUS_SRC_ALPHA";
    case FT_ONE_MINUS_DST_ALPHA: return "ONE_MINUS_DST_ALPHA";
    default:                     return "?";
  }
}

const char *ft_dl_test_name(u32 v)
{
  switch (v)
  {
    case FT_NEVER:    return "NEVER";
    case FT_LESS:     return "LESS";
    case FT_LEQUAL:   return "LEQUAL";
    case FT_GREATER:  return "GREATER";
    case FT_GEQUAL:   return "GEQUAL";
    case FT_EQUAL:    return "EQUAL";
    case FT_NOTEQUAL: return "NOTEQUAL";
    case FT_ALWAYS:   return "ALWAYS";
    default:          return "?";
  }
}

const char *ft_dl_stencil_op_name(u32 v)
{
  switch (v)
  {
    case 0:            return "ZERO";
    case FT_KEEP:      return "KEEP";
    case FT_REPLACE:   return "REPLACE";
    case FT_INCR:      return "INCR";
    case FT_DECR:      return "DECR";
    case FT_INVERT:    return "INVERT";
    case FT_INCR_WRAP: return "INCR_WRAP";
    case FT_DECR_WRAP: return "DECR_WRAP";
    default:           return "?";
  }
}

const char *ft_dl_filter_name(u32 v)
{
  switch (v)
  {
    case FT_NEAREST:  return "NEAREST";
    case FT_BILINEAR: return "BILINEAR";
    default:          return "?";
  }
}

const char *ft_dl_wrap_name(u32 v)
{
  switch (v)
  {
    case FT_BORDER: return "BORDER";
    case FT_REPEAT: return "REPEAT";
    default:        return "?";
  }
}

const char *ft_dl_prim_name(u32 v)
{
  switch (v)
  {
    case FT_BITMAPS:      return "BITMAPS";
    case FT_POINTS:       return "POINTS";
    case FT_LINES:        return "LINES";
    case FT_LINE_STRIP:   return "LINE_STRIP";
    case FT_EDGE_STRIP_R: return "EDGE_STRIP_R";
    case FT_EDGE_STRIP_L: return "EDGE_STRIP_L";
    case FT_EDGE_STRIP_A: return "EDGE_STRIP_A";
    case FT_EDGE_STRIP_B: return "EDGE_STRIP_B";
    case FT_RECTS:        return "RECTS";
    default:              return "?";
  }
}

void ft_dl_print_num_or_name(const char *name, u32 v)
{
  if (name && strcmp(name, "?"))
    printf("%s", name);
  else
    printf("%lu", (unsigned long)v);
}

void ft_dl_print_bitmap_layout(u32 line, u32 w, u32 hi, int has_hi)
{
  u32 format = (w >> 19) & 31UL;
  u32 stride = (w >> 9) & 1023UL;
  u32 height = w & 511UL;

  if (has_hi)
  {
    stride |= (hi & 12UL) << 8;
    height |= (hi & 3UL) << 9;
  }

  printf("%04lX BITMAP_LAYOUT(", (unsigned long)line);
  ft_dl_print_num_or_name(ft_dl_format_name(format), format);
  printf(", %lu, %lu)\r\n", (unsigned long)stride, (unsigned long)height);
}

void ft_dl_print_bitmap_size(u32 line, u32 w, u32 hi, int has_hi)
{
  u32 fxy = (w >> 18) & 7UL;
  u32 filter = (fxy >> 2) & 1UL;
  u32 wrapx = (fxy >> 1) & 1UL;
  u32 wrapy = fxy & 1UL;
  u32 width = (w >> 9) & 511UL;
  u32 height = w & 511UL;

  if (has_hi)
  {
    width |= (hi & 12UL) << 7;
    height |= (hi & 3UL) << 9;
  }

  printf("%04lX BITMAP_SIZE(", (unsigned long)line);
  ft_dl_print_num_or_name(ft_dl_filter_name(filter), filter);
  printf(", ");
  ft_dl_print_num_or_name(ft_dl_wrap_name(wrapx), wrapx);
  printf(", ");
  ft_dl_print_num_or_name(ft_dl_wrap_name(wrapy), wrapy);
  printf(", %lu, %lu)\r\n", (unsigned long)width, (unsigned long)height);
}

void ft_dl_print_bitmap_layout_h(u32 line, u32 w)
{
  printf("%04lX BITMAP_LAYOUT_H(%lu, %lu)\r\n",
         (unsigned long)line,
         (unsigned long)((w & 12UL) >> 2),
         (unsigned long)(w & 3UL));
}

void ft_dl_print_bitmap_size_h(u32 line, u32 w)
{
  printf("%04lX BITMAP_SIZE_H(%lu, %lu)\r\n",
         (unsigned long)line,
         (unsigned long)((w & 12UL) >> 2),
         (unsigned long)(w & 3UL));
}

int ft_dl_print_word(FT_DL_DECODE_CONTEXT *ctx, u32 line, u32 w)
{
  u32 op = w >> 24;
  u32 v;

  if ((w & 0xC0000000UL) == 0x40000000UL)
  {
    printf("%04lX VERTEX2F(", (unsigned long)line);
    ft_dl_print_fixed(ft_dl_sx((w >> 15) & 32767UL, 15), ctx->vertex_format);
    printf(", ");
    ft_dl_print_fixed(ft_dl_sx(w & 32767UL, 15), ctx->vertex_format);
    printf(")\r\n");
    return 0;
  }

  if ((w & 0xC0000000UL) == 0x80000000UL)
  {
    printf("%04lX VERTEX2II(%lu, %lu, %lu, %lu)\r\n",
           (unsigned long)line,
           (unsigned long)((w >> 21) & 511UL),
           (unsigned long)((w >> 12) & 511UL),
           (unsigned long)((w >> 7) & 31UL),
           (unsigned long)(w & 127UL));
    return 0;
  }

  switch (op)
  {
    case 0:
      printf("%04lX DISPLAY()\r\n", (unsigned long)line);
      return 1;

    case 1:
      printf("%04lX BITMAP_SOURCE(0x%06lX)\r\n", (unsigned long)line, (unsigned long)(w & 0x3FFFFFUL));
      return 0;

    case 2:
      printf("%04lX CLEAR_COLOR_RGB(%lu, %lu, %lu)\r\n",
             (unsigned long)line,
             (unsigned long)((w >> 16) & 255UL),
             (unsigned long)((w >> 8) & 255UL),
             (unsigned long)(w & 255UL));
      return 0;

    case 3:
      printf("%04lX TAG(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 255UL));
      return 0;

    case 4:
      printf("%04lX COLOR_RGB(%lu, %lu, %lu)\r\n",
             (unsigned long)line,
             (unsigned long)((w >> 16) & 255UL),
             (unsigned long)((w >> 8) & 255UL),
             (unsigned long)(w & 255UL));
      return 0;

    case 5:
      printf("%04lX BITMAP_HANDLE(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 31UL));
      return 0;

    case 6:
      printf("%04lX CELL(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 127UL));
      return 0;

    case 7:
      ft_dl_print_bitmap_layout(line, w, 0, 0);
      return 0;

    case 8:
      ft_dl_print_bitmap_size(line, w, 0, 0);
      return 0;

    case 9:
      printf("%04lX ALPHA_FUNC(", (unsigned long)line);
      ft_dl_print_num_or_name(ft_dl_test_name((w >> 8) & 7UL), (w >> 8) & 7UL);
      printf(", %lu)\r\n", (unsigned long)(w & 255UL));
      return 0;

    case 10:
      printf("%04lX STENCIL_FUNC(", (unsigned long)line);
      ft_dl_print_num_or_name(ft_dl_test_name((w >> 16) & 7UL), (w >> 16) & 7UL);
      printf(", %lu, %lu)\r\n", (unsigned long)((w >> 8) & 255UL), (unsigned long)(w & 255UL));
      return 0;

    case 11:
      printf("%04lX BLEND_FUNC(", (unsigned long)line);
      ft_dl_print_num_or_name(ft_dl_blend_name((w >> 3) & 7UL), (w >> 3) & 7UL);
      printf(", ");
      ft_dl_print_num_or_name(ft_dl_blend_name(w & 7UL), w & 7UL);
      printf(")\r\n");
      return 0;

    case 12:
      printf("%04lX STENCIL_OP(", (unsigned long)line);
      ft_dl_print_num_or_name(ft_dl_stencil_op_name((w >> 3) & 7UL), (w >> 3) & 7UL);
      printf(", ");
      ft_dl_print_num_or_name(ft_dl_stencil_op_name(w & 7UL), w & 7UL);
      printf(")\r\n");
      return 0;

    case 13:
      ft_dl_print_fixed_cmd_1(line, "POINT_SIZE", (i32)(w & 8191UL), 4);
      return 0;

    case 14:
      ft_dl_print_fixed_cmd_1(line, "LINE_WIDTH", (i32)(w & 4095UL), 4);
      return 0;

    case 15:
      printf("%04lX CLEAR_COLOR_A(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 255UL));
      return 0;

    case 16:
      printf("%04lX COLOR_A(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 255UL));
      return 0;

    case 17:
      printf("%04lX CLEAR_STENCIL(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 255UL));
      return 0;

    case 18:
      printf("%04lX CLEAR_TAG(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 255UL));
      return 0;

    case 19:
      printf("%04lX STENCIL_MASK(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 255UL));
      return 0;

    case 20:
      printf("%04lX TAG_MASK(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 1UL));
      return 0;

    case 21:
      printf("%04lX BITMAP_TRANSFORM_A(%ld)\r\n", (unsigned long)line, (long)ft_dl_sx(w & 131071UL, 17));
      return 0;

    case 22:
      printf("%04lX BITMAP_TRANSFORM_B(%ld)\r\n", (unsigned long)line, (long)ft_dl_sx(w & 131071UL, 17));
      return 0;

    case 23:
      printf("%04lX BITMAP_TRANSFORM_C(%ld)\r\n", (unsigned long)line, (long)ft_dl_sx(w & 16777215UL, 24));
      return 0;

    case 24:
      printf("%04lX BITMAP_TRANSFORM_D(%ld)\r\n", (unsigned long)line, (long)ft_dl_sx(w & 131071UL, 17));
      return 0;

    case 25:
      printf("%04lX BITMAP_TRANSFORM_E(%ld)\r\n", (unsigned long)line, (long)ft_dl_sx(w & 131071UL, 17));
      return 0;

    case 26:
      printf("%04lX BITMAP_TRANSFORM_F(%ld)\r\n", (unsigned long)line, (long)ft_dl_sx(w & 16777215UL, 24));
      return 0;

    case 27:
      printf("%04lX SCISSOR_XY(%lu, %lu)\r\n",
             (unsigned long)line,
             (unsigned long)((w >> 9) & 511UL),
             (unsigned long)(w & 511UL));
      return 0;

    case 28:
      printf("%04lX SCISSOR_SIZE(%lu, %lu)\r\n",
             (unsigned long)line,
             (unsigned long)((w >> 12) & 4095UL),
             (unsigned long)(w & 4095UL));
      return 0;

    case 29:
      printf("%04lX CALL(0x%03lX)\r\n", (unsigned long)line, (unsigned long)(w & 2047UL));
      return 0;

    case 30:
      printf("%04lX JUMP(0x%03lX)\r\n", (unsigned long)line, (unsigned long)(w & 2047UL));
      return 0;

    case 31:
      v = w & 15UL;
      printf("%04lX BEGIN(", (unsigned long)line);
      ft_dl_print_num_or_name(ft_dl_prim_name(v), v);
      printf(")\r\n");
      return 0;

    case 32:
      printf("%04lX COLOR_MASK(%lu, %lu, %lu, %lu)\r\n",
             (unsigned long)line,
             (unsigned long)((w >> 3) & 1UL),
             (unsigned long)((w >> 2) & 1UL),
             (unsigned long)((w >> 1) & 1UL),
             (unsigned long)(w & 1UL));
      return 0;

    case 33:
      printf("%04lX END()\r\n", (unsigned long)line);
      return 0;

    case 34:
      ft_dl_context_save(ctx);
      printf("%04lX SAVE_CONTEXT()\r\n", (unsigned long)line);
      return 0;

    case 35:
      ft_dl_context_restore(ctx);
      printf("%04lX RESTORE_CONTEXT()\r\n", (unsigned long)line);
      return 0;

    case 36:
      printf("%04lX RETURN()\r\n", (unsigned long)line);
      return 0;

    case 37:
      printf("%04lX MACRO(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 1UL));
      return 0;

    case 38:
      printf("%04lX CLEAR(%lu, %lu, %lu)\r\n",
             (unsigned long)line,
             (unsigned long)((w >> 2) & 1UL),
             (unsigned long)((w >> 1) & 1UL),
             (unsigned long)(w & 1UL));
      return 0;

    case 39:
      ctx->vertex_format = (u8)(w & 7UL);
      printf("%04lX VERTEX_FORMAT(%lu)\r\n", (unsigned long)line, (unsigned long)(w & 7UL));
      return 0;

    case 40:
      ft_dl_print_bitmap_layout_h(line, w);
      return 0;

    case 41:
      ft_dl_print_bitmap_size_h(line, w);
      return 0;

    case 42:
      printf("%04lX PALETTE_SOURCE(0x%06lX)\r\n", (unsigned long)line, (unsigned long)(w & 0x3FFFFFUL));
      return 0;

    case 43:
      ft_dl_print_fixed_cmd_1(line, "VERTEX_TRANSLATE_X", ft_dl_sx(w & 131071UL, 17), 4);
      return 0;

    case 44:
      ft_dl_print_fixed_cmd_1(line, "VERTEX_TRANSLATE_Y", ft_dl_sx(w & 131071UL, 17), 4);
      return 0;
  }

  printf("%04lX UNKNOWN(0x%08lX)\r\n", (unsigned long)line, (unsigned long)w);
  return 0;
}

void ft_dl_flush_pending(u32 *line, int *has_layout_hi, u32 layout_hi, int *has_size_hi, u32 size_hi)
{
  if (*has_layout_hi)
  {
    ft_dl_print_bitmap_layout_h(*line, layout_hi);
    (*line)++;
    *has_layout_hi = 0;
  }

  if (*has_size_hi)
  {
    ft_dl_print_bitmap_size_h(*line, size_hi);
    (*line)++;
    *has_size_hi = 0;
  }
}

void ft_dl_decode(const u8 *buf, u32 bytes)
{
  FT_DL_DECODE_CONTEXT ctx;
  u32 out_line = 0;
  u32 raw_line;
  u32 layout_hi = 0;
  u32 size_hi = 0;
  int has_layout_hi = 0;
  int has_size_hi = 0;

  ft_dl_context_init(&ctx);

  for (raw_line = 0; raw_line < bytes / 4; raw_line++)
  {
    u32 w = ft_rd32le(buf + raw_line * 4);
    u32 op = w >> 24;

    if (op == 40)
    {
      if (has_layout_hi)
        ft_dl_flush_pending(&out_line, &has_layout_hi, layout_hi, &has_size_hi, size_hi);

      layout_hi = w;
      has_layout_hi = 1;
      continue;
    }

    if (op == 41)
    {
      if (has_size_hi)
        ft_dl_flush_pending(&out_line, &has_layout_hi, layout_hi, &has_size_hi, size_hi);

      size_hi = w;
      has_size_hi = 1;
      continue;
    }

    if (op == 7 && has_layout_hi)
    {
      if (has_size_hi)
        ft_dl_flush_pending(&out_line, &has_layout_hi, layout_hi, &has_size_hi, size_hi);

      ft_dl_print_bitmap_layout(out_line, w, layout_hi, 1);
      out_line++;
      has_layout_hi = 0;
      continue;
    }

    if (op == 8 && has_size_hi)
    {
      if (has_layout_hi)
        ft_dl_flush_pending(&out_line, &has_layout_hi, layout_hi, &has_size_hi, size_hi);

      ft_dl_print_bitmap_size(out_line, w, size_hi, 1);
      out_line++;
      has_size_hi = 0;
      continue;
    }

    ft_dl_flush_pending(&out_line, &has_layout_hi, layout_hi, &has_size_hi, size_hi);

    if (ft_dl_print_word(&ctx, out_line, w))
      break;

    out_line++;
  }
}

esp_err_t ft_dl_line_swap_wait()
{
  ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_LINE);
  return ft_wait_swap(1000);
}

esp_err_t ft_dl_read_current(void *buf, u32 bytes)
{
  esp_err_t err;
  esp_err_t err_restore;

  err = ft_dl_line_swap_wait();
  if (err != ESP_OK) return err;

  err = ft_read(buf, FT_RAM_DL, bytes);

  err_restore = ft_dl_line_swap_wait();
  if (err == ESP_OK)
    err = err_restore;

  return err;
}

int ft_swap_cmd(int argc, char **)
{
  esp_err_t err;
  esp_err_t err2;

  if (argc != 2)
  {
    printf("Usage:\r\n");
    printf("  ft swap\r\n");
    return 1;
  }

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT swap failed: %d\r\n", (int)err);
    return 1;
  }

  printf("FT swap: DLSWAP=%u\r\n", (unsigned)FT_DLSWAP_FRAME);
  return 0;
}

int ft_dl_cmd(int argc, char **argv)
{
  esp_err_t err;
  esp_err_t err2;
  u32 dl = 0;
  int current = 1;
  int ok;

  if (argc > 3)
  {
    printf("Usage:\r\n");
    printf("  ft dl\r\n");
    printf("  ft dl 0\r\n");
    printf("  ft dl 1\r\n");
    return 1;
  }

  if (argc == 3)
  {
    dl = ft_parse_num_arg(argv[2], "dl", &ok);
    if (!ok)
      return 1;

    if (dl > 1)
    {
      printf("Bad <dl>: %lu, allowed 0-1\r\n", (unsigned long)dl);
      return 1;
    }

    current = dl == 0;
  }

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  if (current)
    err = ft_dl_read_current(cmdl, FT_DL_READ_SIZE);
  else
    err = ft_read(cmdl, FT_RAM_DL, FT_DL_READ_SIZE);

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT dl read failed: %d\r\n", (int)err);
    return 1;
  }

  if (current)
    printf("\r\nFT DL current:\r\n");
  else
    printf("\r\nFT DL staging:\r\n");

  ft_dl_decode(cmdl, FT_DL_READ_SIZE);

  return 0;
}

int ft_demo_logo_cmd()
{
  esp_err_t err;
  esp_err_t err2;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  ft_ccmd_start(cmdl);
  ft_Logo();

  err = ft_ccmd_write();
  if (err == ESP_OK)
    err = ft_cp_wait(5000);

  err2 = ft_close_session();
  if (err == ESP_OK && err2 != ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT demo 7 failed: %s (0x%x)\r\n",
      esp_err_to_name(err), (unsigned int)err);
    return 1;
  }

  return 0;
}

int ft_demo_cmd(int argc, char **argv)
{
  u32 num;
  int ok;

  if (argc != 3)
  {
    printf("Usage:\r\n");
    printf("  ft demo <num>\r\n");
    printf("Modes:\r\n");
    printf("  0  Test screen\r\n");
    printf("  1  Test sequence\r\n");
    printf("  2  Bitmap render\r\n");
    printf("  3  4bpp mode\r\n");
    printf("  4  Attribute mode\r\n");
    printf("  5  ZX Spectrum mode\r\n");
    printf("  6  RGB888 mode\r\n");
    printf("  7  CMD_LOGO\r\n");
    return 1;
  }

  num = ft_parse_num_arg(argv[2], "num", &ok);
  if (!ok)
    return 1;

  if (ft_current_mode < 0)
    printf("FT demo warning: mode is not set, run 'ft mode <0-%lu>' first\r\n", (unsigned long)FT_MODE_MAX);

  switch (num)
  {
    case 0:
    {
      esp_err_t err = ft_draw_calib_frame();
      if (err != ESP_OK)
      {
        printf("FT demo 0 failed: %s (0x%x)\r\n",
          esp_err_to_name(err), (unsigned int)err);
        return 1;
      }
      return 0;
    }

    case 1:
      return ft_demo_bases_cmd();

    case 2:
      return ft_demo_frac_cmd();

    case 3:
      return ft_demo_4bpp_mode_cmd();

    case 4:
      return ft_demo_attr_mode_cmd();

    case 5:
      return ft_demo_zx_mode_cmd();

    case 6:
      return ft_demo_rgb888_cmd();

    case 7:
      return ft_demo_logo_cmd();
  }

  printf("Unknown demo number: %lu\r\n", (unsigned long)num);
  return 1;
}

int ft_cli_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  ft res\r\n");
    printf("  ft mode <0-%lu>\r\n", (unsigned long)FT_MODE_MAX);
    printf("  ft info\r\n");
    printf("  ft dump [addr]\r\n");
    printf("  ft dl [0|1]\r\n");
    printf("  ft swap\r\n");
    printf("  ft wreg <addr> <value>\r\n");
    printf("  ft wr <addr> <value>        (auto 8/16/32)\r\n");
    printf("  ft wr8 <addr> <value>\r\n");
    printf("  ft wr16 <addr> <value>\r\n");
    printf("  ft wr32 <addr> <value>\r\n");
    printf("  ft demo <num>\r\n");
    printf("  ft jpg <path>\r\n");
    printf("  ft dxp <path>\r\n");
    printf("  ft spi <1|2|4>\r\n");
    printf("  ft freq [<MHz>]\r\n");
    printf("  ft perf\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "res"))
    return ft_res_cmd(argc, argv);

  if (!strcmp(op, "mode"))
    return ft_mode_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return ft_info_cmd(argc, argv);

  if (!strcmp(op, "wreg"))
    return ft_wreg_cmd(argc, argv);

  if (!strcmp(op, "wr") || !strcmp(op, "wr8") ||
      !strcmp(op, "wr16") || !strcmp(op, "wr32"))
    return ft_wr_cmd(argc, argv);

  if (!strcmp(op, "dump"))
    return ft_dump_cmd(argc, argv);

  if (!strcmp(op, "dl"))
    return ft_dl_cmd(argc, argv);

  if (!strcmp(op, "swap"))
    return ft_swap_cmd(argc, argv);

  if (!strcmp(op, "demo"))
    return ft_demo_cmd(argc, argv);

  if (!strcmp(op, "jpg"))
    return ft_jpg_cmd(argc, argv);

  if (!strcmp(op, "dxp"))
    return ft_dxp_cmd(argc, argv);

  if (!strcmp(op, "spi"))
    return ft_spi_cmd(argc, argv);

  if (!strcmp(op, "freq"))
    return ft_freq_cmd(argc, argv);

  if (!strcmp(op, "perf"))
    return ft_perf_cmd(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void ft_console_register_system_commands()
{
  {
    const esp_console_cmd_t cmd =
    {
      .command  = "ft",
      .help     = "FT812 commands: res/mode/info/dump/dl/swap/wreg/wr/wr8/wr16/wr32/demo/jpg/dxp/spi/freq/perf",
      .hint     = NULL,
      .func     = &ft_cli_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }
}
