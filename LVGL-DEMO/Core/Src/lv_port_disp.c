/*
 * lv_port_disp.c
 *
 *  Created on: Jan 29, 2026
 *      Author: Lenin
 *
 * ── Fixes applied ────────────────────────────────────────────────────────────
 *
 * BUG 1 (FATAL) — LV_DRAW_SW_SUPPORT_RGB565_SWAPPED was 0 in lv_conf.h.
 *   lv_display_set_color_format(RGB565_SWAPPED) has no effect when the SW
 *   renderer for that format is compiled out — LVGL produces no pixels and
 *   the display stays at its power-on blue.
 *   FIX: set LV_DRAW_SW_SUPPORT_RGB565_SWAPPED = 1 in lv_conf.h  AND
 *        call lv_display_set_color_format(LV_COLOR_FORMAT_RGB565_SWAPPED)
 *        here.  LVGL now renders pixels already byte-swapped, which is the
 *        big-endian RGB565 order ILI9341 expects over SPI.
 *
 * BUG 2 (FATAL) — ConvHL() in ILI9341_DrawBitmap() mutated the LVGL draw
 *   buffer IN PLACE.  In double-buffer mode LVGL reuses the same pointer for
 *   the next strip render.  Pixels LVGL does not re-render in that pass are
 *   still byte-swapped from the previous ConvHL call, so ConvHL swaps them a
 *   second time → wrong colours / pixelated garbage frame after frame.
 *   FIX: ConvHL removed from ILI9341_DrawBitmap() and ILI9341_DrawBitmapDMA()
 *        in ili9341.c.  Buffer arrives already in correct wire order.
 *
 * BUG 3 (MINOR / SRAM WASTE) — Buffer size used sizeof(lv_color_t) = 4.
 *   In LVGL 9, lv_color_t is always 32-bit regardless of LV_COLOR_DEPTH,
 *   so each buffer was 9 600 B instead of the intended 4 800 B, wasting
 *   ~10 KB of SRAM and giving lv_display_set_buffers() a wrong byte count.
 *   FIX: explicit * 2U (2 bytes per RGB565 pixel).
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "lv_port_disp.h"
#include "ILI9341.h"

/* Display resolution */
#define MY_DISP_HOR_RES  240
#define MY_DISP_VER_RES  320

/*
 * Draw buffers — 2 bytes per pixel (RGB565).
 * 10 rows × 240 px × 2 B = 4 800 B each → 9 600 B total, comfortably
 * within the STM32F401's 64 KB SRAM alongside the 32 KB LVGL heap.
 * Raise DISP_BUF_ROWS to 20 if rendering is slow and SRAM allows.
 */
#define DISP_BUF_ROWS  10
#define DISP_BUF_SIZE  (MY_DISP_HOR_RES * DISP_BUF_ROWS * 2U)

static lv_display_t *disp;

static uint8_t buf_1[DISP_BUF_SIZE];
static uint8_t buf_2[DISP_BUF_SIZE];

static void disp_init(void);
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area,
                       uint8_t *px_map);

/* ── Public init ─────────────────────────────────────────────────────────── */
void lv_port_disp_init(void)
{
    disp_init();

    disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);

    /*
     * LV_COLOR_FORMAT_RGB565_SWAPPED instructs LVGL to store each 16-bit
     * pixel with its low byte first in the buffer.  When SPI clocks bytes
     * out MSB-first, that low byte hits the wire first, and ILI9341
     * receives it as the high byte of the colour word — correct big-endian
     * RGB565 without any runtime byte-swap.
     *
     * REQUIREMENT: LV_DRAW_SW_SUPPORT_RGB565_SWAPPED must be 1 in lv_conf.h
     * otherwise this call has no effect and you get an all-blue screen.
     */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);

    lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_display_set_flush_cb(disp, disp_flush);
}

/* ── Hardware init ───────────────────────────────────────────────────────── */
static void disp_init(void)
{
    ILI9341_Init();
}

/* ── Flush callback (no DMA) ─────────────────────────────────────────────── */
/*
 * px_map arrives in ILI9341 wire order (RGB565_SWAPPED).
 * ILI9341_DrawBitmap() no longer calls ConvHL(), so it sends the buffer
 * straight over SPI with no modification.
 */
static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area,
                       uint8_t *px_map)
{
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    ILI9341_SetWindow(area->x1, area->y1, area->x2, area->y2);
    ILI9341_DrawBitmap((uint16_t)w, (uint16_t)h, px_map);

    lv_display_flush_ready(disp_drv);
}

/* ── DMA version (optional) ──────────────────────────────────────────────── */
/*
 * To use DMA, replace the disp_flush body above with this and make sure
 * lv_display_flush_ready() is called from HAL_SPI_TxCpltCallback().
 *
 * static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area,
 *                         uint8_t *px_map)
 * {
 *     int32_t w = area->x2 - area->x1 + 1;
 *     int32_t h = area->y2 - area->y1 + 1;
 *     ILI9341_SetWindow(area->x1, area->y1, area->x2, area->y2);
 *     ILI9341_DrawBitmapDMA((uint16_t)w, (uint16_t)h, px_map);
 *     // Do NOT call lv_display_flush_ready() here
 * }
 *
 * void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
 * {
 *     if (hspi->Instance == SPI1)
 *         lv_display_flush_ready(disp);
 * }
 */
