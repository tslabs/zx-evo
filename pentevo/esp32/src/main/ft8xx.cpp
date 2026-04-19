
#include <string.h>
#include <stdlib.h>
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
#include "spi_slave.h"
#include "ft8xx.h"

const char TAG[] = "ft8xx";

void hexdump(const void *data, size_t len, uint64_t base_off);

#define CHUNK_PAYLOAD DMA_BUF_SIZE

// #define FT_DEMO_BITMAP_W       640
// #define FT_DEMO_BITMAP_H       480
#define FT_DEMO_BITMAP_W       320
#define FT_DEMO_BITMAP_H       240
#define FT_DEMO_BITMAP_SIZE    (FT_DEMO_BITMAP_W * FT_DEMO_BITMAP_H)
#define FT_DEMO_BITMAP0_ADDR   FT_RAM_G
#define FT_DEMO_BITMAP1_ADDR   (FT_RAM_G + FT_DEMO_BITMAP_SIZE)
#define FT_DEMO_PALETTE_ADDR   (FT_DEMO_BITMAP1_ADDR + FT_DEMO_BITMAP_SIZE)
#define FT_DEMO_PALETTE_SIZE   (256UL * 4UL)

#define FT_DEMO_TEXTVGA_ADDR             (FT_DEMO_PALETTE_ADDR + FT_DEMO_PALETTE_SIZE)
#define FT_DEMO_TEXTVGA_BUF_SIZE         (16UL * 1024UL)
#define FT_DEMO_TEXTVGA_BG_ADDR          (FT_DEMO_TEXTVGA_ADDR + FT_DEMO_TEXTVGA_BUF_SIZE)
#define FT_DEMO_TEXTVGA_LINE_STRIDE      256
#define FT_DEMO_TEXTVGA_CELL_SIZE        2
#define FT_DEMO_TEXTVGA_VISIBLE_COLS     80
#define FT_DEMO_TEXTVGA_VISIBLE_ROWS     30
#define FT_DEMO_TEXTVGA_CELL_W           8
#define FT_DEMO_TEXTVGA_CELL_H           16
#define FT_DEMO_TEXTVGA_VISIBLE_W        (FT_DEMO_TEXTVGA_VISIBLE_COLS * FT_DEMO_TEXTVGA_CELL_W)
#define FT_DEMO_TEXTVGA_VISIBLE_H        (FT_DEMO_TEXTVGA_VISIBLE_ROWS * FT_DEMO_TEXTVGA_CELL_H)
#define FT_DEMO_TEXTVGA_TOTAL_ROWS       (FT_DEMO_TEXTVGA_BUF_SIZE / FT_DEMO_TEXTVGA_LINE_STRIDE)
#define FT_DEMO_TEXTVGA_SCREEN_X         ((800 - FT_DEMO_TEXTVGA_VISIBLE_W) / 2)
#define FT_DEMO_TEXTVGA_SCREEN_Y         ((600 - FT_DEMO_TEXTVGA_VISIBLE_H) / 2)


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

u32 *ft_ccmdb = nullptr;
u16 ft_ccmdp = 0;
EXT_RAM_BSS_ATTR u8 cmdl[8192];
u8 ft_spi_width = 2;
u32 ft_spi_freq_hz = 20000000UL;

const FT_MODE ft_modes[] =                         //  |  # | visible  | Fpix MHz | clocks/line | lines/frame | line kHz | frame Hz |
{                                                  //  | -- | -------- | -------- | ----------- | ----------- | -------- | -------- |
  {6,  2,  16,  96,  48,  640, 11, 2, 31,  480},   //  |  0 | 640x480  |       24 |         800 |         524 |   30.000 |   57.252 |
  {8,  2,  24,  40, 128,  640,  9, 3, 28,  480},   //  |  1 | 640x480  |       32 |         832 |         520 |   38.462 |   73.964 |
  {8,  2,  16,  96,  48,  640, 11, 2, 31,  480},   //  |  2 | 640x480  |       32 |         800 |         524 |   40.000 |   76.336 |
  {5,  1,  40, 128,  88,  800,  1, 4, 23,  600},   //  |  3 | 800x600  |       40 |        1056 |         628 |   37.879 |   60.317 |
  {10, 2,  40, 128,  88,  800,  1, 4, 23,  600},   //  |  4 | 800x600  |       40 |        1056 |         628 |   37.879 |   60.317 |
  {6,  1,  56, 120,  64,  800, 37, 6, 23,  600},   //  |  5 | 800x600  |       48 |        1040 |         666 |   46.154 |   69.300 |
  {7,  1,  32,  64, 152,  800,  1, 3, 27,  600},   //  |  6 | 800x600  |       56 |        1048 |         631 |   53.435 |   84.683 |
  {8,  1,  24, 136, 160, 1024,  3, 6, 29,  768},   //  |  7 | 1024x768 |       64 |        1344 |         806 |   47.619 |   59.081 |
  {9,  1,  24, 136, 144, 1024,  3, 6, 29,  768},   //  |  8 | 1024x768 |       72 |        1328 |         806 |   54.217 |   67.267 |
  {10, 1,  16,  96, 176, 1024,  1, 3, 28,  768},   //  |  9 | 1024x768 |       80 |        1312 |         800 |   60.976 |   76.220 |
  {7,  1,  24,  56, 124,  640,  1, 3, 38, 1024},   //  | 10 | 640x1024 |       56 |         844 |        1066 |   66.351 |   62.243 |
  {9,  1, 110,  40, 220, 1280,  5, 5, 20,  720},   //  | 11 | 1280x720 |       72 |        1650 |         750 |   43.636 |   58.182 |
  {9,  1,  93,  40, 187, 1280,  5, 5, 20,  720},   //  | 12 | 1280x720 |       72 |        1600 |         750 |   45.000 |   60.000 |
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
  ft_ccmd((28UL << 24) | ((width & 1023L) << 10) | ((height & 1023L) << 0));
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
  if (m >= (sizeof(ft_modes) / sizeof(ft_modes[0])))
    return ESP_ERR_INVALID_ARG;

  const FT_MODE *mode = &ft_modes[m];
  esp_err_t err;

  err = ft_switch_spi_to_1_bit();
  if (err != ESP_OK) return err;

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
  u32 value;
  int ok;

  if (argc < 4)
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
         (unsigned long)addr,
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
  (void)argc;
  (void)argv;

  esp_err_t err;
  u8 chip_id = 0;
  u8 chip_type[4] = {};
  char datestamp[17] = {};
  u8 b[256];

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_read_detect(chip_id, chip_type, datestamp);
  if (err != ESP_OK) printf("FT detect failed: %d\r\n", (int)err);
  printf("\r\n");
  ft_print_detect(chip_id, chip_type, datestamp);

  printf("\r\nRegister hexdump:\r\n");
  ft_read(b, FT_REG_ID, sizeof(b));
  hexdump(b, sizeof(b), FT_REG_ID);

  err = ft_close_session();

  if (err != ESP_OK)
  {
    printf("FT dump close failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

esp_err_t ft_demo_draw_mode_1024_768()
{
  const char *mode_txt = "1024x768@59Hz (64MHz)";
  esp_err_t err;
  esp_err_t err2;
  u16 i;
  u16 j;

  err = ft_open_session();
  if (err != ESP_OK)
    return err;

  err = ft_set_mode(FT_MODE_1024_768_59);
  if (err == ESP_OK)
    err = ft_cp_reset();

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
    ft_Begin(FT_POINTS);
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

  err = ft_set_mode(FT_MODE_800_600_60_80MHZ);

  if (err == ESP_OK)
    err = ft_cp_reset();

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
      ft_Begin(FT_POINTS);
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
  u32 pixels = FT_DEMO_BITMAP_W * FT_DEMO_BITMAP_H;

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
  
  for (u32 y = 0; y < FT_DEMO_BITMAP_H; y++)
  {
    for (u32 x = 0; x < FT_DEMO_BITMAP_W; x++)
    {
      u32 idx = y * FT_DEMO_BITMAP_W + x;
      dst[idx] = x * 256 / FT_DEMO_BITMAP_W + offs;
    }
  }
  
  offs++;
}

void ft_demo_frac_render1(FT_DEMO_FRAC_STATE *st, u8 *dst, u32 frame_no)
{
  if (!st || !dst) return;
  if (!frame_no) return;

  u32 iter_mark = frame_no;

  for (u32 y = 0; y < FT_DEMO_BITMAP_H; y++)
  {
    float ci = -1.5f + (float)y * 3.0f / (float)FT_DEMO_BITMAP_H;

    for (u32 x = 0; x < FT_DEMO_BITMAP_W; x++)
    {
      u32 idx = y * FT_DEMO_BITMAP_W + x;

      if (!st->alive[idx])
        continue;

      float cr = -2.0f + (float)x * 3.0f / (float)FT_DEMO_BITMAP_W;
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
  return ft_write(src, ft_addr, FT_DEMO_BITMAP_SIZE);
}

esp_err_t ft_demo_frac_upload_bitmap_bg(const void *src, u32 ft_addr)
{
  return spi_master_write_buf_bg((u8)(((ft_addr >> 16) & 0x3F) | 0x80), (u16)ft_addr, src, FT_DEMO_BITMAP_SIZE);
}

esp_err_t ft_demo_frac_show_bitmap(u32 bmp_addr, u32 frame_no)
{
  esp_err_t err;
  u32 iters = frame_no;
  const u16 screen_w = 800;
  const u16 screen_h = 600;
  const i32 scale_x = ((i32)FT_DEMO_BITMAP_W << 8) / screen_w;
  const i32 scale_y = ((i32)FT_DEMO_BITMAP_H << 8) / screen_h;
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
  ft_BitmapLayout(FT_PALETTED8, (u16)FT_DEMO_BITMAP_W, (u16)FT_DEMO_BITMAP_H);
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

  ft_ColorRGB(255, 255, 0);
  ft_Text(8, screen_h - 48, 18, 0, "Mode: 800x600x8");

  ft_ColorRGB(0, 255, 255);
  ft_Text(8, screen_h - 32, 18, 0, "Frame:");
  ft_Number(120, screen_h - 32, 18, 0, (i32)frame_no);

  ft_Text(8, screen_h - 16, 18, 0, "Iters:");
  ft_Number(120, screen_h - 16, 18, 0, (i32)iters);

  while (((u32)ft_ccmdp << 2) < 2048)
    ft_ColorA(255);

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
  u32 active_addr = FT_DEMO_BITMAP0_ADDR;
  u32 inactive_addr = FT_DEMO_BITMAP1_ADDR;
  u32 shown_frame = 1;
  u32 prepared_frame = 2;
  int upload_buf_idx = 1;

  render_bufs[0] = (u8*)heap_caps_malloc(FT_DEMO_BITMAP_SIZE, MALLOC_CAP_INTERNAL);
  render_bufs[1] = (u8*)heap_caps_malloc(FT_DEMO_BITMAP_SIZE, MALLOC_CAP_INTERNAL);

  if (!render_bufs[0] || !render_bufs[1])
  {
    printf("FT demo render buffer alloc failed, size=%lu\r\n", (unsigned long)FT_DEMO_BITMAP_SIZE);
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

  memset(render_bufs[1], 0, FT_DEMO_BITMAP_SIZE);

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    ft_demo_frac_state_free(&st);
    free(render_bufs[0]);
    free(render_bufs[1]);
    return 1;
  }

  err = ft_set_mode(FT_MODE_800_600_60_80MHZ);

  ft_demo_frac_make_palette(palette);

  err = ft_write(palette, FT_DEMO_PALETTE_ADDR, FT_DEMO_PALETTE_SIZE);

  ft_demo_frac_render(&st, render_bufs[0], shown_frame);
  ft_demo_frac_render(&st, render_bufs[1], prepared_frame);

  err = ft_demo_frac_upload_bitmap_bg(render_bufs[0], active_addr);

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

i16 ft_demo_road_clip_coord(i32 v, i16 max_v)
{
  if (v < 0) return 0;
  if (v > (i32)max_v) return max_v;
  return (i16)v;
}

esp_err_t ft_demo_road_show_frame(u32 frame_no)
{
  esp_err_t err;
  const i32 screen_w = 800;
  const i32 screen_h = 600;

  i32 sway = rsin(84, (u16)(frame_no * 163UL));
  i32 bob = rsin(18, (u16)(frame_no * 97UL));
  i32 curve = rsin(180, (u16)(frame_no * 71UL));
  i32 vp_x = (screen_w / 2) + sway / 3;
  i32 vp_y = 168 + bob / 6;
  i32 horizon_y = vp_y + 34;
  u32 dash_phase = frame_no >> 1;

  ft_ccmd_start(cmdl);

  ft_Dlstart();
  ft_VertexFormat(0);

  ft_ClearColorRGB(0, 0, 0);
  ft_ClearColorA(255);
  ft_Clear(1, 1, 1);

  ft_Gradient(0, 0, 0x081018, 0, horizon_y, 0x203860);
  ft_Gradient(0, horizon_y, 0x0a0c10, 0, screen_h - 1, 0x020202);

  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);
  ft_Begin(FT_POINTS);

  ft_ColorA(48);
  ft_ColorRGB(0, 180, 255);
  ft_PointSize(180 << 4);
  ft_Vertex2f(ft_demo_road_clip_coord(vp_x, screen_w - 1), ft_demo_road_clip_coord(vp_y + 10, screen_h - 1));

  ft_ColorA(40);
  ft_ColorRGB(255, 80, 200);
  ft_PointSize(72 << 4);
  ft_Vertex2f(ft_demo_road_clip_coord(vp_x + curve / 12, screen_w - 1), ft_demo_road_clip_coord(vp_y + 12, screen_h - 1));
  ft_End();

  for (u32 i = 0; i < 18; i++)
  {
    float phase = (float)((frame_no * 28UL + i * 57UL) & 1023UL) / 1023.0f;
    float p = phase * phase;

    i32 cx = vp_x + (i32)((float)curve * p * 0.45f);
    i32 cy = vp_y + 10 + (i32)(p * 36.0f);
    i32 hw = 26 + (i32)(p * 430.0f);
    i32 hh = 16 + (i32)(p * 300.0f);
    u16 line_w = (u16)((2.0f + p * 7.0f) * 16.0f);
    u8 alpha = (u8)(32.0f + p * 160.0f);

    i16 x0 = ft_demo_road_clip_coord(cx - hw, screen_w - 1);
    i16 y0 = ft_demo_road_clip_coord(cy - hh, screen_h - 1);
    i16 x1 = ft_demo_road_clip_coord(cx + hw, screen_w - 1);
    i16 y1 = ft_demo_road_clip_coord(cy + hh, screen_h - 1);

    ft_BlendFunc(FT_SRC_ALPHA, FT_ONE);
    ft_ColorA(alpha);
    ft_LineWidth(line_w);

    if (i & 1)
      ft_ColorRGB(0, 220, 255);
    else
      ft_ColorRGB(255, 60, 180);

    ft_Begin(FT_LINE_STRIP);
    ft_Vertex2f(x0, y0);
    ft_Vertex2f(x1, y0);
    ft_Vertex2f(x1, y1);
    ft_Vertex2f(x0, y1);
    ft_Vertex2f(x0, y0);
    ft_End();
  }

  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE_MINUS_SRC_ALPHA);
  ft_ColorA(255);
  ft_Begin(FT_RECTS);

  for (u32 i = 0; i < 34; i++)
  {
    float f0 = (float)i / 34.0f;
    float f1 = (float)(i + 1) / 34.0f;
    float p0 = f0 * f0;
    float p1 = f1 * f1;

    i32 y0 = horizon_y + (i32)(p0 * (float)(screen_h - horizon_y));
    i32 y1 = horizon_y + (i32)(p1 * (float)(screen_h - horizon_y));
    i32 cx = vp_x + (i32)((float)curve * p1 * p1 * 0.55f);
    i32 road_half = 18 + (i32)(p1 * 300.0f);
    i32 curb_half = road_half + 10 + (i32)(p1 * 84.0f);
    i32 marker_half = 2 + (i32)(p1 * 7.0f);
    i32 glow_half = 2 + (i32)(p1 * 4.0f);

    i16 left = ft_demo_road_clip_coord(cx - road_half, screen_w - 1);
    i16 right = ft_demo_road_clip_coord(cx + road_half, screen_w - 1);
    i16 curb_left = ft_demo_road_clip_coord(cx - curb_half, screen_w - 1);
    i16 curb_right = ft_demo_road_clip_coord(cx + curb_half, screen_w - 1);
    i16 yy0 = ft_demo_road_clip_coord(y0, screen_h - 1);
    i16 yy1 = ft_demo_road_clip_coord(y1, screen_h - 1);

    ft_ColorRGB((u8)(18 + i * 2), (u8)(10 + i), (u8)(32 + i * 3));
    ft_Vertex2f(curb_left, yy0);
    ft_Vertex2f(left, yy1);

    ft_ColorRGB((u8)(18 + i * 2), (u8)(10 + i), (u8)(32 + i * 3));
    ft_Vertex2f(right, yy0);
    ft_Vertex2f(curb_right, yy1);

    ft_ColorRGB((u8)(18 + i * 3), (u8)(18 + i * 3), (u8)(24 + i * 4));
    ft_Vertex2f(left, yy0);
    ft_Vertex2f(right, yy1);

    if (((i + dash_phase) & 7UL) < 2UL)
    {
      i16 marker_left = ft_demo_road_clip_coord(cx - marker_half, screen_w - 1);
      i16 marker_right = ft_demo_road_clip_coord(cx + marker_half, screen_w - 1);
      ft_ColorRGB(255, 240, 96);
      ft_Vertex2f(marker_left, yy0);
      ft_Vertex2f(marker_right, yy1);
    }

    if (((i + dash_phase) & 3UL) == 0)
    {
      i16 glow_left0 = ft_demo_road_clip_coord(curb_left - glow_half, screen_w - 1);
      i16 glow_left1 = ft_demo_road_clip_coord(curb_left + glow_half, screen_w - 1);
      i16 glow_right0 = ft_demo_road_clip_coord(curb_right - glow_half, screen_w - 1);
      i16 glow_right1 = ft_demo_road_clip_coord(curb_right + glow_half, screen_w - 1);

      ft_ColorRGB(0, 220, 255);
      ft_Vertex2f(glow_left0, yy0);
      ft_Vertex2f(glow_left1, yy1);

      ft_ColorRGB(255, 60, 180);
      ft_Vertex2f(glow_right0, yy0);
      ft_Vertex2f(glow_right1, yy1);
    }
  }

  ft_End();

  ft_ColorA(255);
  ft_BlendFunc(FT_SRC_ALPHA, FT_ONE_MINUS_SRC_ALPHA);
  ft_ColorRGB(255, 220, 80);
  ft_Text(8, 8, 18, 0, "FT demo 3 - pseudo 3D tunnel road");

  ft_ColorRGB(0, 255, 255);
  ft_Text(8, 28, 18, 0, "Frame:");
  ft_Number(88, 28, 18, 0, (i32)frame_no);

  ft_ColorRGB(200, 200, 200);
  ft_Text(8, 48, 18, 0, "Press any key to stop");

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

int ft_demo_road_cmd()
{
  esp_err_t err;
  esp_err_t err2;
  u32 frame_no = 0;

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    return 1;
  }

  err = ft_set_mode(FT_MODE_800_600_60_80MHZ);
  if (err == ESP_OK)
    err = ft_cp_reset();

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);

    while (1)
    {
      err = ft_demo_road_show_frame(frame_no);
      if (err != ESP_OK)
      {
        printf("FT demo_road error: draw failed: %s (0x%x)\r\n", esp_err_to_name(err), (unsigned int)err);
        break;
      }

      err = ft_wait_swap(1000);
      if (err != ESP_OK)
      {
        printf("FT demo_road error: ft_wait_swap(1000) failed: %s (0x%x)\r\n", esp_err_to_name(err), (unsigned int)err);
        break;
      }

      char c;
      if (uart_read_bytes(UART_NUM_0, &c, 1, 0) > 0)
      {
        printf("FT demo stopped\r\n");
        break;
      }

      frame_no++;
    }
  }

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  if (err != ESP_OK)
  {
    printf("FT demo_road failed: %d\r\n", (int)err);
    return 1;
  }

  return 0;
}

u8 ft_demo_textvga_cur_ink = 15;
u8 ft_demo_textvga_cur_paper = 0;
u8 *ft_demo_textvga_fg_addr = nullptr;
u8 *ft_demo_textvga_bg_addr = nullptr;

void ft_demo_textvga_set_attr(u8 ink, u8 paper)
{
  ft_demo_textvga_cur_ink = (u8)(ink & 15U);
  ft_demo_textvga_cur_paper = (u8)(paper & 15U);
}

void ft_demo_textvga_set_addrs(u8 *fg, u8 *bg)
{
  ft_demo_textvga_fg_addr = fg;
  ft_demo_textvga_bg_addr = bg;
}

void ft_demo_textvga_clear_ex(u8 *buf, u8 ch, u8 attr)
{
  if (!buf) return;

  memset(buf, 0, FT_DEMO_TEXTVGA_BUF_SIZE);

  for (u32 y = 0; y < FT_DEMO_TEXTVGA_TOTAL_ROWS; y++)
  {
    u8 *row = buf + y * FT_DEMO_TEXTVGA_LINE_STRIDE;

    for (u32 x = 0; x < (FT_DEMO_TEXTVGA_LINE_STRIDE / FT_DEMO_TEXTVGA_CELL_SIZE); x++)
    {
      row[x * FT_DEMO_TEXTVGA_CELL_SIZE + 0] = ch;
      row[x * FT_DEMO_TEXTVGA_CELL_SIZE + 1] = (u8)(attr & 15U);
    }
  }
}

void ft_demo_textvga_clear(u8 *buf, u8 attr)
{
  ft_demo_textvga_clear_ex(buf, ' ', attr);
}

void ft_demo_textvga_put_raw(u8 *buf, int x, int y, u8 ch, u8 attr)
{
  u32 off;
  u8 *row;

  if (!buf) return;
  if (x < 0 || x >= FT_DEMO_TEXTVGA_VISIBLE_COLS) return;
  if (y < 0 || y >= FT_DEMO_TEXTVGA_VISIBLE_ROWS) return;

  row = buf + (u32)y * FT_DEMO_TEXTVGA_LINE_STRIDE;
  off = (u32)x * FT_DEMO_TEXTVGA_CELL_SIZE;
  row[off + 0] = ch;
  row[off + 1] = (u8)(attr & 15U);
}

void ft_demo_textvga_putc(int x, int y, u8 ch)
{
  if (!ft_demo_textvga_fg_addr || !ft_demo_textvga_bg_addr) return;

  ft_demo_textvga_put_raw(ft_demo_textvga_bg_addr, x, y, 0xDB, ft_demo_textvga_cur_paper);
  ft_demo_textvga_put_raw(ft_demo_textvga_fg_addr, x, y, ch, ft_demo_textvga_cur_ink);
}

void ft_demo_textvga_fill_rect(u8 *buf, int x, int y, int w, int h, u8 ch, u8 attr)
{
  if (!buf) return;
  if (w <= 0 || h <= 0) return;

  for (int yy = 0; yy < h; yy++)
  {
    int py = y + yy;

    if (py < 0 || py >= FT_DEMO_TEXTVGA_VISIBLE_ROWS)
      continue;

    for (int xx = 0; xx < w; xx++)
      ft_demo_textvga_put_raw(buf, x + xx, py, ch, attr);
  }
}

void ft_demo_textvga_fill_paper(u8 *buf, int x, int y, int w, int h, u8 paper)
{
  ft_demo_textvga_fill_rect(buf, x, y, w, h, 0xDB, (u8)(paper & 15U));
}

void ft_demo_textvga_draw_box2(int x, int y, int w, int h)
{
  if (!ft_demo_textvga_fg_addr || !ft_demo_textvga_bg_addr) return;
  if (w < 2 || h < 2) return;

  ft_demo_textvga_putc(x,         y,         0xC9);
  ft_demo_textvga_putc(x + w - 1, y,         0xBB);
  ft_demo_textvga_putc(x,         y + h - 1, 0xC8);
  ft_demo_textvga_putc(x + w - 1, y + h - 1, 0xBC);

  for (int xx = 1; xx < (w - 1); xx++)
  {
    ft_demo_textvga_putc(x + xx, y,         0xCD);
    ft_demo_textvga_putc(x + xx, y + h - 1, 0xCD);
  }

  for (int yy = 1; yy < (h - 1); yy++)
  {
    ft_demo_textvga_putc(x,         y + yy, 0xBA);
    ft_demo_textvga_putc(x + w - 1, y + yy, 0xBA);
  }
}

void ft_demo_textvga_puts(int x, int y, const char *s)
{
  if (!ft_demo_textvga_fg_addr || !ft_demo_textvga_bg_addr || !s) return;
  if (y < 0 || y >= FT_DEMO_TEXTVGA_VISIBLE_ROWS) return;

  while (*s && x < FT_DEMO_TEXTVGA_VISIBLE_COLS)
  {
    if (x >= 0)
      ft_demo_textvga_putc(x, y, (u8)*s);

    x++;
    s++;
  }
}

char ft_demo_hex_digit(u8 v)
{
  v &= 15;

  if (v < 10)
    return (char)('0' + v);

  return (char)('A' + (v - 10));
}

void ft_demo_textvga_put_hex1(int x, int y, u8 v)
{
  ft_demo_textvga_putc(x, y, (u8)ft_demo_hex_digit(v));
}

void ft_demo_textvga_put_hex2(int x, int y, u8 v)
{
  ft_demo_textvga_putc(x + 0, y, (u8)ft_demo_hex_digit((u8)(v >> 4)));
  ft_demo_textvga_putc(x + 1, y, (u8)ft_demo_hex_digit(v));
}

void ft_demo_textvga_fill()
{
  const int left_x = 2;
  const int right_x = 42;

  if (!ft_demo_textvga_fg_addr || !ft_demo_textvga_bg_addr) return;

  ft_demo_textvga_clear(ft_demo_textvga_fg_addr, 15);
  ft_demo_textvga_clear(ft_demo_textvga_bg_addr, 0);
  ft_demo_textvga_fill_paper(ft_demo_textvga_bg_addr, 0, 0, FT_DEMO_TEXTVGA_VISIBLE_COLS, FT_DEMO_TEXTVGA_VISIBLE_ROWS, 0);

  ft_demo_textvga_set_attr(15, 0);
  ft_demo_textvga_draw_box2(0, 0, FT_DEMO_TEXTVGA_VISIBLE_COLS, FT_DEMO_TEXTVGA_VISIBLE_ROWS);

  ft_demo_textvga_set_attr(14, 0);
  ft_demo_textvga_puts(left_x, 1, "TEXTVGA color/code demo");

  ft_demo_textvga_set_attr(11, 0);
  ft_demo_textvga_puts(left_x, 2, "top=ink chars, bottom=0xDB paper layer");

  {
    int x0 = left_x;
    int y0 = 11;

    ft_demo_textvga_set_attr(15, 0);
    for (int ink = 0; ink < 16; ink++)
      ft_demo_textvga_put_hex1(x0 + 4 + ink * 2, y0, (u8)ink);

    for (int paper = 0; paper < 16; paper++)
    {
      ft_demo_textvga_set_attr(15, (u8)paper);
      ft_demo_textvga_put_hex1(x0 + 1, y0 + 1 + paper, (u8)paper);

      for (int ink = 0; ink < 16; ink++)
      {
        ft_demo_textvga_set_attr((u8)ink, (u8)paper);
        ft_demo_textvga_putc(x0 + 4 + ink * 2, y0 + 1 + paper, 'A');
      }
    }
  }

  {
    int x0 = right_x;
    int y0 = 11;

    ft_demo_textvga_set_attr(15, 0);
    for (int col = 0; col < 16; col++)
      ft_demo_textvga_put_hex1(x0 + 3 + col * 2, y0, (u8)col);

    for (int row = 0; row < 16; row++)
    {
      ft_demo_textvga_put_hex1(x0 + 1, y0 + 1 + row, (u8)row);

      ft_demo_textvga_set_attr(7, 0);
      for (int col = 0; col < 16; col++)
        ft_demo_textvga_putc(x0 + 3 + col * 2, y0 + 1 + row, (u8)(row * 16 + col));

      ft_demo_textvga_set_attr(15, 0);
    }
  }
}

esp_err_t ft_demo_textvga_show()
{
  esp_err_t err;

  ft_ccmd_start(cmdl);

  ft_Dlstart();
  ft_VertexFormat(0);

  ft_ClearColorRGB(0, 0, 32);
  ft_ClearColorA(255);
  ft_Clear(1, 1, 1);

  ft_ColorRGB(255, 255, 255);
  ft_ColorA(255);

  ft_BitmapHandle(0);
  ft_BitmapSource(FT_DEMO_TEXTVGA_BG_ADDR);
  ft_BitmapLayout(FT_TEXTVGA, FT_DEMO_TEXTVGA_LINE_STRIDE, FT_DEMO_TEXTVGA_VISIBLE_ROWS);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, FT_DEMO_TEXTVGA_VISIBLE_W, FT_DEMO_TEXTVGA_VISIBLE_H);

  ft_BitmapHandle(1);
  ft_BitmapSource(FT_DEMO_TEXTVGA_ADDR);
  ft_BitmapLayout(FT_TEXTVGA, FT_DEMO_TEXTVGA_LINE_STRIDE, FT_DEMO_TEXTVGA_VISIBLE_ROWS);
  ft_BitmapSize(FT_NEAREST, FT_BORDER, FT_BORDER, FT_DEMO_TEXTVGA_VISIBLE_W, FT_DEMO_TEXTVGA_VISIBLE_H);

  ft_Begin(FT_BITMAPS);
  ft_Vertex2ii(FT_DEMO_TEXTVGA_SCREEN_X, FT_DEMO_TEXTVGA_SCREEN_Y, 0, 0);
  ft_Vertex2ii(FT_DEMO_TEXTVGA_SCREEN_X, FT_DEMO_TEXTVGA_SCREEN_Y, 1, 0);
  ft_End();

  ft_Display();
  ft_Swap();

  err = ft_ccmd_write();
  if (err != ESP_OK) return err;

  err = ft_cp_wait(1000);
  if (err != ESP_OK) return err;

  return ESP_OK;
}

int ft_demo_textvga_cmd()
{
  esp_err_t err;
  esp_err_t err2;
  u8 *fg = nullptr;
  u8 *bg = nullptr;

  fg = (u8*)heap_caps_malloc(FT_DEMO_TEXTVGA_BUF_SIZE, MALLOC_CAP_INTERNAL);
  bg = (u8*)heap_caps_malloc(FT_DEMO_TEXTVGA_BUF_SIZE, MALLOC_CAP_INTERNAL);
  if (!fg || !bg)
  {
    printf("FT TEXTVGA buffer alloc failed, size=%lu x2\r\n", (unsigned long)FT_DEMO_TEXTVGA_BUF_SIZE);
    if (fg) free(fg);
    if (bg) free(bg);
    return 1;
  }

  ft_demo_textvga_set_addrs(fg, bg);
  ft_demo_textvga_fill();

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("FT open failed: %d\r\n", (int)err);
    free(fg);
    free(bg);
    return 1;
  }

  err = ft_set_mode(FT_MODE_800_600_60_80MHZ);
  if (err == ESP_OK)
    err = ft_cp_reset();

  if (err == ESP_OK)
    err = ft_write(bg, FT_DEMO_TEXTVGA_BG_ADDR, FT_DEMO_TEXTVGA_BUF_SIZE);

  if (err == ESP_OK)
    err = ft_write(fg, FT_DEMO_TEXTVGA_ADDR, FT_DEMO_TEXTVGA_BUF_SIZE);

  if (err == ESP_OK)
  {
    ft_rreg8(FT_REG_INT_FLAGS);
    err = ft_demo_textvga_show();
  }

  if (err == ESP_OK)
    err = ft_wait_swap(1000);

  err2 = ft_close_session();
  if (err == ESP_OK)
    err = err2;

  free(fg);
  free(bg);

  if (err != ESP_OK)
  {
    printf("FT demo_textvga failed: %d\r\n", (int)err);
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
    printf("  1  Test sequence\r\n");
    printf("  2  Bitmap render demo\r\n");
    printf("  3  Pseudo 3D tunnel road\r\n");
    printf("  4  TEXTVGA 80x30 text screen\r\n");
    return 1;
  }

  num = ft_parse_num_arg(argv[2], "num", &ok);
  if (!ok)
    return 1;

  switch (num)
  {
    case 0:
    {
      esp_err_t err = ft_demo_draw_mode_1024_768();
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
      return ft_demo_road_cmd();

    case 4:
      return ft_demo_textvga_cmd();
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
    printf("  ft info\r\n");
    printf("  ft dump\r\n");
    printf("  ft wreg <addr> <value>\r\n");
    printf("  ft demo <num>\r\n");
    printf("  ft spi <1|2|4>\r\n");
    printf("  ft freq [<MHz>]\r\n");
    printf("  ft perf\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "res"))
    return ft_res_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return ft_info_cmd(argc, argv);

  if (!strcmp(op, "wreg"))
    return ft_wreg_cmd(argc, argv);

  if (!strcmp(op, "dump"))
    return ft_dump_cmd(argc, argv);

  if (!strcmp(op, "demo"))
    return ft_demo_cmd(argc, argv);

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
      .help     = "FT812 commands: res/info/dump/demo/spi/freq/perf",
      .hint     = NULL,
      .func     = &ft_cli_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }
}
