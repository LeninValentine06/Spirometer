/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "lvgl.h"
#include "lv_port_disp.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    float flow;
    float total_volume;
    float fev1;
    float fev6;
    float fvc;
    float fev1_fvc;
    float pef;
    float fvl;
} spirometry_data_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * Chart configuration
 * -------------------
 * 60 points × 100 ms = 6 s rolling window (standard spirometry duration).
 *
 * Y axis uses AUTO-SCALING:
 *   - Initial range   : 0–6 L  (CHART_Y_INIT_MAX scaled units)
 *   - Grows in steps  : CHART_Y_STEP_L litres at a time
 *   - Chart stores     : (int32_t)(volume_L × CHART_SCALE)
 *
 * X axis rolls: always shows [T_end-6 .. T_end] seconds.
 */
#define CHART_POINTS        60      /* samples in the rolling window           */
#define CHART_SCALE         10      /* int scale factor  (1 unit = 0.1 L)      */
#define CHART_Y_INIT_MAX    60      /* initial Y ceiling = 6.0 L × CHART_SCALE */
#define CHART_Y_STEP_L      2       /* grow Y ceiling in 2 L increments         */
#define CHART_Y_STEP        (CHART_Y_STEP_L * CHART_SCALE)  /* in scaled units */

/* Layout (inside cont2, 320×120 px, pad=5)
 *   [left labels 115 px][y_scale 22 px][chart 155 px] ← right edge
 *   x_scale strip 14 px below chart
 */
#define CHART_W             155
#define CHART_H             80
#define Y_SCALE_W           22
#define X_SCALE_H           14

/* Tick counts */
#define X_TICK_LABEL_COUNT  7   /* 0..6 s — one per second */
#define Y_TICK_LABEL_COUNT  5   /* evenly spaced, rebuilt whenever Y range grows */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
typedef struct
{
    /* Metric display labels (Container 1) */
    lv_obj_t *flow;
    lv_obj_t *tot_vol;
    lv_obj_t *fev1;
    lv_obj_t *fev6;

    /* Metric display labels (Container 2 – left column) */
    lv_obj_t *fvc;
    lv_obj_t *fev1_fvc;
    lv_obj_t *pef;
    lv_obj_t *fvl;

    /* Chart */
    lv_obj_t          *chart;
    lv_chart_series_t *chart_series;
    uint16_t           chart_point_index;   /* ring position 0..CHART_POINTS-1  */
    uint32_t           total_sample_count;  /* absolute sample counter           */
    int32_t            chart_y_max;         /* current Y ceiling (scaled units)  */
    bool               maneuver_active;

    /* X-axis scale widget + rolling label buffers */
    lv_obj_t *x_scale;
    char      x_tick_bufs[X_TICK_LABEL_COUNT][6];
    const char *x_tick_ptrs[X_TICK_LABEL_COUNT + 1]; /* NULL-terminated for lv_scale */

    /* Y-axis scale widget + dynamic label buffers
     * Labels are rebuilt whenever the Y ceiling grows.               */
    lv_obj_t *y_scale;
    char      y_tick_bufs[Y_TICK_LABEL_COUNT][6];
    const char *y_tick_ptrs[Y_TICK_LABEL_COUNT + 1];

    /* Live cursor readout ("X.Xs  Y.YYL") */
    lv_obj_t *cursor_label;
} ui_context_t;

static ui_context_t ui;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void ui_init(void);
void ui_update(const spirometry_data_t *data);
void ui_reset_chart(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();

  /* USER CODE BEGIN 2 */
  lv_init();
  lv_tick_set_cb(HAL_GetTick);
  lv_port_disp_init();
  ui_init();

  spirometry_data_t data    = {0};
  uint32_t          last_update = 0;
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* USER CODE BEGIN 3 */
      uint32_t current_tick = HAL_GetTick();

      if (current_tick - last_update >= 100)
      {
          last_update = current_tick;

          data.flow         += 0.1f;
          data.total_volume += 0.05f;
          data.fev1         += 0.02f;
          data.fev6         += 0.02f;
          data.fvc          += 0.03f;
          data.fev1_fvc      = 80.0f;
          data.pef           = data.flow * 1.2f;
          data.fvl           = data.total_volume * data.flow;

          ui_update(&data);
      }

      lv_timer_handler();
      HAL_Delay(5);
      /* USER CODE END 3 */
  }
  /* USER CODE END WHILE */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM            = 8;
  RCC_OscInitStruct.PLL.PLLN            = 84;
  RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ            = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

/* USER CODE BEGIN 4 */

/* -------------------------------------------------------------------------
 * ui_rebuild_y_labels
 *
 * Rebuild the Y-axis tick label strings and re-apply them to the scale
 * widget.  Called once at init and again whenever chart_y_max grows.
 *
 * Y_TICK_LABEL_COUNT evenly-spaced ticks from 0 to current ceiling (L).
 * Example: ceiling = 6 L  → labels "0", "2", "4", "6" (if count = 4)
 *          ceiling = 8 L  → labels "0", "2", "4", "6", "8" (count = 5)
 * ------------------------------------------------------------------------- */
static void ui_rebuild_y_labels(void)
{
    /* ceiling in real litres */
    float ceil_L = (float)ui.chart_y_max / (float)CHART_SCALE;

    for (int i = 0; i < Y_TICK_LABEL_COUNT; i++)
    {
        /* distribute labels evenly: tick i represents fraction i/(count-1) */
        float val_L = ceil_L * (float)i / (float)(Y_TICK_LABEL_COUNT - 1);
        snprintf(ui.y_tick_bufs[i], sizeof(ui.y_tick_bufs[i]),
                 "%.0f", val_L);
        ui.y_tick_ptrs[i] = ui.y_tick_bufs[i];
    }
    ui.y_tick_ptrs[Y_TICK_LABEL_COUNT] = NULL;  /* sentinel */

    /* Push updated labels and range to the scale widget */
    lv_scale_set_range(ui.y_scale, 0, ui.chart_y_max);
    lv_scale_set_text_src(ui.y_scale, (const char **)ui.y_tick_ptrs);
    lv_obj_invalidate(ui.y_scale);
}

/* -------------------------------------------------------------------------
 * ui_reset_chart — call at the start of a new breath maneuver
 * ------------------------------------------------------------------------- */
void ui_reset_chart(void)
{
    lv_chart_set_all_value(ui.chart, ui.chart_series, 0);
    ui.chart_point_index  = 0;
    ui.total_sample_count = 0;
    ui.maneuver_active    = true;

    /* Reset Y ceiling back to initial range */
    ui.chart_y_max = CHART_Y_INIT_MAX;
    lv_chart_set_range(ui.chart, LV_CHART_AXIS_PRIMARY_Y, 0, ui.chart_y_max);
    ui_rebuild_y_labels();

    /* Reset X axis labels to 0..6 */
    for (int i = 0; i < X_TICK_LABEL_COUNT; i++)
        snprintf(ui.x_tick_bufs[i], sizeof(ui.x_tick_bufs[i]), "%d", i);
    lv_scale_set_text_src(ui.x_scale, (const char **)ui.x_tick_ptrs);
    lv_obj_invalidate(ui.x_scale);
}

/* -------------------------------------------------------------------------
 * ui_init
 * ------------------------------------------------------------------------- */
void ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_MAX, LV_PART_MAIN);
    lv_obj_set_style_text_color(scr, lv_color_white(), 0);

    /* ---- Container 1 (top, metrics) ---------------------------------- */
    lv_obj_t *cont1 = lv_obj_create(scr);
    lv_obj_set_size(cont1, 320, 120);
    lv_obj_set_pos(cont1, 0, 0);
    lv_obj_set_style_bg_color(cont1, lv_color_make(20, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont1, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont1, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont1, 5, LV_PART_MAIN);

    ui.flow = lv_label_create(cont1);
    lv_obj_align(ui.flow, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(ui.flow, "Flow: 0.00 L/s");

    ui.tot_vol = lv_label_create(cont1);
    lv_obj_align(ui.tot_vol, LV_ALIGN_TOP_LEFT, 5, 30);
    lv_label_set_text(ui.tot_vol, "Vol: 0.00 L");

    ui.fev1 = lv_label_create(cont1);
    lv_obj_align(ui.fev1, LV_ALIGN_TOP_LEFT, 5, 55);
    lv_label_set_text(ui.fev1, "FEV1: 0.00 L");

    ui.fev6 = lv_label_create(cont1);
    lv_obj_align(ui.fev6, LV_ALIGN_TOP_LEFT, 5, 80);
    lv_label_set_text(ui.fev6, "FEV6: 0.00 L");

    /* ---- Container 2 (bottom, left labels + chart) ------------------- */
    lv_obj_t *cont2 = lv_obj_create(scr);
    lv_obj_set_size(cont2, 320, 120);
    lv_obj_set_pos(cont2, 0, 120);
    lv_obj_set_style_bg_color(cont2, lv_color_make(20, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont2, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont2, 5, LV_PART_MAIN);

    /* Left-column metric labels — capped at 115 px to avoid chart overlap */
    ui.fvc = lv_label_create(cont2);
    lv_obj_align(ui.fvc, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_set_width(ui.fvc, 115);
    lv_label_set_text(ui.fvc, "FVC: 0.00 L");

    ui.fev1_fvc = lv_label_create(cont2);
    lv_obj_align(ui.fev1_fvc, LV_ALIGN_TOP_LEFT, 5, 30);
    lv_obj_set_width(ui.fev1_fvc, 115);
    lv_label_set_text(ui.fev1_fvc, "FEV1/FVC:\n0.0 %");

    ui.pef = lv_label_create(cont2);
    lv_obj_align(ui.pef, LV_ALIGN_TOP_LEFT, 5, 60);
    lv_obj_set_width(ui.pef, 115);
    lv_label_set_text(ui.pef, "PEF: 0.00 L/s");

    ui.fvl = lv_label_create(cont2);
    lv_obj_align(ui.fvl, LV_ALIGN_TOP_LEFT, 5, 85);
    lv_obj_set_width(ui.fvl, 115);
    lv_label_set_text(ui.fvl, "FVL: 0.00");

    /* ---- Chart -------------------------------------------------------
     * Layout inside cont2 (pad=5):
     *   x=115  y_scale (22 px wide)
     *   x=137  chart   (155 px wide, 80 px tall)
     *   y=86   x_scale (14 px tall, same width as chart)
     * ------------------------------------------------------------------ */
    ui.chart = lv_chart_create(cont2);
    lv_obj_set_size(ui.chart, CHART_W, CHART_H);
    lv_obj_set_pos(ui.chart, 137, 5);

    lv_chart_set_type(ui.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ui.chart, CHART_POINTS);
    lv_chart_set_update_mode(ui.chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(ui.chart, 4, 6); /* 4 h-lines, 6 v-lines */

    ui.chart_y_max = CHART_Y_INIT_MAX;
    lv_chart_set_range(ui.chart, LV_CHART_AXIS_PRIMARY_Y, 0, ui.chart_y_max);

    lv_obj_set_style_bg_color(ui.chart, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(ui.chart, lv_color_make(100, 100, 100), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.chart, 1, LV_PART_MAIN);
    lv_obj_set_style_line_color(ui.chart, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_line_width(ui.chart, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui.chart, 0, LV_PART_MAIN);

    ui.chart_series = lv_chart_add_series(ui.chart, lv_color_make(0, 255, 0),
                                          LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(ui.chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(ui.chart, 0, 0, LV_PART_INDICATOR);
    lv_chart_set_all_value(ui.chart, ui.chart_series, 0);

    ui.chart_point_index  = 0;
    ui.total_sample_count = 0;
    ui.maneuver_active    = false;

    /* ---- Y-axis lv_scale (vertical-left of chart) --------------------
     * The scale range and labels are driven by ui_rebuild_y_labels().
     * Y_TICK_LABEL_COUNT major ticks, no minor ticks between them.
     * ------------------------------------------------------------------ */
    ui.y_scale = lv_scale_create(cont2);
    lv_scale_set_mode(ui.y_scale, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_obj_set_size(ui.y_scale, Y_SCALE_W, CHART_H);
    lv_obj_set_pos(ui.y_scale, 115, 5);

    /* Total ticks = Y_TICK_LABEL_COUNT, all major (no minor subdivisions) */
    lv_scale_set_total_tick_count(ui.y_scale, Y_TICK_LABEL_COUNT);
    lv_scale_set_major_tick_every(ui.y_scale, 1);

    lv_obj_set_style_bg_opa(ui.y_scale, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.y_scale, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui.y_scale, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(ui.y_scale, &lv_font_montserrat_10, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(ui.y_scale, lv_color_make(120, 120, 120), LV_PART_INDICATOR);
    lv_obj_set_style_length(ui.y_scale, 4, LV_PART_INDICATOR);
    lv_obj_set_style_length(ui.y_scale, 0, LV_PART_ITEMS); /* hide minor ticks */

    /* Y unit label ("L") above the scale */
    lv_obj_t *y_unit = lv_label_create(cont2);
    lv_label_set_text(y_unit, "L");
    lv_obj_set_style_text_font(y_unit, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(y_unit, lv_color_make(180, 180, 180), 0);
    lv_obj_set_pos(y_unit, 119, 0);

    /* Build initial Y tick labels (0..6 L) */
    ui_rebuild_y_labels();

    /* ---- X-axis lv_scale (horizontal-bottom of chart) ----------------
     * 13 total ticks: major every 2nd → 7 labelled ticks (one per second).
     * Labels are updated dynamically in ui_update() as time advances.
     * ------------------------------------------------------------------ */
    ui.x_scale = lv_scale_create(cont2);
    lv_scale_set_mode(ui.x_scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    lv_obj_set_size(ui.x_scale, CHART_W, X_SCALE_H);
    lv_obj_set_pos(ui.x_scale, 137, 5 + CHART_H);

    lv_scale_set_range(ui.x_scale, 0, 6);
    lv_scale_set_total_tick_count(ui.x_scale, 13); /* minor every 0.5 s */
    lv_scale_set_major_tick_every(ui.x_scale, 2);  /* major every 1 s   */

    /* Initialise X label buffers and wire the pointer array */
    for (int i = 0; i < X_TICK_LABEL_COUNT; i++)
    {
        snprintf(ui.x_tick_bufs[i], sizeof(ui.x_tick_bufs[i]), "%d", i);
        ui.x_tick_ptrs[i] = ui.x_tick_bufs[i];
    }
    ui.x_tick_ptrs[X_TICK_LABEL_COUNT] = NULL;
    lv_scale_set_text_src(ui.x_scale, (const char **)ui.x_tick_ptrs);

    lv_obj_set_style_bg_opa(ui.x_scale, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.x_scale, 0, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui.x_scale, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_text_font(ui.x_scale, &lv_font_montserrat_10, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(ui.x_scale, lv_color_make(120, 120, 120), LV_PART_INDICATOR);
    lv_obj_set_style_line_color(ui.x_scale, lv_color_make(70, 70, 70), LV_PART_ITEMS);
    lv_obj_set_style_length(ui.x_scale, 4, LV_PART_INDICATOR);
    lv_obj_set_style_length(ui.x_scale, 2, LV_PART_ITEMS);

    /* X unit label ("s") to the right of the scale */
    lv_obj_t *x_unit = lv_label_create(cont2);
    lv_label_set_text(x_unit, "s");
    lv_obj_set_style_text_font(x_unit, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(x_unit, lv_color_make(180, 180, 180), 0);
    lv_obj_set_pos(x_unit, 137 + CHART_W + 2, 5 + CHART_H);

    /* ---- Chart title ------------------------------------------------- */
    lv_obj_t *chart_title = lv_label_create(cont2);
    lv_label_set_text(chart_title, "Vol-Time");
    lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(chart_title, lv_color_make(180, 180, 180), 0);
    lv_obj_set_pos(chart_title, 137 + CHART_W / 2 - 20, 0);

    /* ---- Live cursor readout ----------------------------------------- */
    ui.cursor_label = lv_label_create(cont2);
    lv_label_set_text(ui.cursor_label, "0.0s 0.00L");
    lv_obj_set_style_text_font(ui.cursor_label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(ui.cursor_label, lv_color_make(0, 220, 255), 0);
    lv_obj_set_pos(ui.cursor_label, 137 + CHART_W - 58, 7);
}

/* -------------------------------------------------------------------------
 * ui_update — called every 100 ms
 * ------------------------------------------------------------------------- */
void ui_update(const spirometry_data_t *data)
{
    if (data == NULL) return;

    char buf[32];

    /* ---- Update metric labels ---------------------------------------- */
    snprintf(buf, sizeof(buf), "Flow: %.2f L/s",   data->flow);
    lv_label_set_text(ui.flow, buf);

    snprintf(buf, sizeof(buf), "Vol: %.2f L",       data->total_volume);
    lv_label_set_text(ui.tot_vol, buf);

    snprintf(buf, sizeof(buf), "FEV1: %.2f L",      data->fev1);
    lv_label_set_text(ui.fev1, buf);

    snprintf(buf, sizeof(buf), "FEV6: %.2f L",      data->fev6);
    lv_label_set_text(ui.fev6, buf);

    snprintf(buf, sizeof(buf), "FVC: %.2f L",       data->fvc);
    lv_label_set_text(ui.fvc, buf);

    snprintf(buf, sizeof(buf), "FEV1/FVC:\n%.1f %%", data->fev1_fvc);
    lv_label_set_text(ui.fev1_fvc, buf);

    snprintf(buf, sizeof(buf), "PEF: %.2f L/s",     data->pef);
    lv_label_set_text(ui.pef, buf);

    snprintf(buf, sizeof(buf), "FVL: %.2f",          data->fvl);
    lv_label_set_text(ui.fvl, buf);

    /* ---- Push sample into rolling chart ------------------------------ */
    int32_t scaled = (int32_t)(data->total_volume * CHART_SCALE + 0.5f);
    if (scaled < 0) scaled = 0;

    /*
     * Y auto-scale: if the incoming value would exceed the current ceiling,
     * grow the ceiling in CHART_Y_STEP increments until it fits, then
     * update both the chart range and the Y-axis tick labels.
     */
    if (scaled > ui.chart_y_max)
    {
        while (scaled > ui.chart_y_max)
            ui.chart_y_max += CHART_Y_STEP;

        lv_chart_set_range(ui.chart, LV_CHART_AXIS_PRIMARY_Y,
                           0, ui.chart_y_max);
        ui_rebuild_y_labels();  /* <-- live Y axis update */
    }

    lv_chart_set_next_value(ui.chart, ui.chart_series, scaled);

    ui.chart_point_index = (ui.chart_point_index + 1) % CHART_POINTS;
    ui.total_sample_count++;

    /* ---- Update rolling X-axis tick labels ---------------------------
     * Visible window: [T_end - 6 .. T_end] seconds.
     * tick[i] = T_end - 6 + i,  clamped to >= 0.
     * Labels are only rewritten when their text actually changes.
     * ------------------------------------------------------------------ */
    {
        float t_end = (float)ui.total_sample_count * 0.1f;
        bool  changed = false;

        for (int i = 0; i < X_TICK_LABEL_COUNT; i++)
        {
            float t = t_end - 6.0f + (float)i;
            if (t < 0.0f) t = 0.0f;

            char tmp[6];
            if (t >= 99.5f)
                snprintf(tmp, sizeof(tmp), "99+");
            else
                snprintf(tmp, sizeof(tmp), "%d", (int)(t + 0.5f));

            if (strcmp(tmp, ui.x_tick_bufs[i]) != 0)
            {
                memcpy(ui.x_tick_bufs[i], tmp, sizeof(tmp));
                changed = true;
            }
        }

        if (changed)
        {
            lv_scale_set_text_src(ui.x_scale, (const char **)ui.x_tick_ptrs);
            lv_obj_invalidate(ui.x_scale);
        }
    }

    /* ---- Live cursor readout ----------------------------------------- */
    float time_s = (float)ui.total_sample_count * 0.1f;
    snprintf(buf, sizeof(buf), "%.1fs %.2fL", time_s, data->total_volume);
    lv_label_set_text(ui.cursor_label, buf);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1) {}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
