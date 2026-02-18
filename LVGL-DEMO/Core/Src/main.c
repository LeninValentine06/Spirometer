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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef struct
{
    lv_obj_t *flow;
    lv_obj_t *tot_vol;
    lv_obj_t *fev1;
    lv_obj_t *fev6;
    lv_obj_t *fvc;
    lv_obj_t *fev1_fvc;
    lv_obj_t *pef;
    lv_obj_t *fvl;
    lv_obj_t *chart;
    lv_chart_series_t *chart_series;
} ui_context_t;

static ui_context_t ui;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void ui_init();
void ui_update(const spirometry_data_t *data);
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

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  // Initialize LVGL
  lv_init();

  // Set up tick interface (as per LVGL 9.x documentation)
  lv_tick_set_cb(HAL_GetTick);

  // Initialize display driver
  lv_port_disp_init();
  ui_init();

  // Initialize spirometry data
  spirometry_data_t data = {0};
  uint32_t last_update = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* USER CODE BEGIN 3 */

      uint32_t current_tick = HAL_GetTick();

      if(current_tick - last_update >= 100)
      {
          last_update = current_tick;

          data.flow += 0.1f;
          data.total_volume += 0.05f;
          data.fev1 += 0.02f;
          data.fev6 += 0.02f;
          data.fvc += 0.03f;
          data.fev1_fvc = 80.0f;
          data.pef = data.flow * 1.2f;
          data.fvl = data.total_volume * data.flow;

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void ui_init(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* Screen background */
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_MAX, LV_PART_MAIN);

    /* Text color (apply to screen so children inherit) */
    lv_obj_set_style_text_color(scr, lv_color_white(), 0);

    /* Container 1 - Top half with labels */
    lv_obj_t * cont1 = lv_obj_create(scr);
    lv_obj_set_size(cont1, 320, 120);
    lv_obj_set_pos(cont1, 0, 0);
    lv_obj_set_style_bg_color(cont1, lv_color_make(20, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont1, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont1, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont1, 5, LV_PART_MAIN);

    /* Container 2 - Bottom half with labels and chart */
    lv_obj_t * cont2 = lv_obj_create(scr);
    lv_obj_set_size(cont2, 320, 120);
    lv_obj_set_pos(cont2, 0, 120);
    lv_obj_set_style_bg_color(cont2, lv_color_make(20, 20, 20), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(cont2, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont2, 5, LV_PART_MAIN);

    /* Labels in Container 1 */
    /* FLOW */
    ui.flow = lv_label_create(cont1);
    lv_obj_align(ui.flow, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(ui.flow, "Flow: 0.00 L/s");

    /* Total Volume */
    ui.tot_vol = lv_label_create(cont1);
    lv_obj_align(ui.tot_vol, LV_ALIGN_TOP_LEFT, 5, 30);
    lv_label_set_text(ui.tot_vol, "Total Volume: 0.00 L");

    /* FEV1 */
    ui.fev1 = lv_label_create(cont1);
    lv_obj_align(ui.fev1, LV_ALIGN_TOP_LEFT, 5, 55);
    lv_label_set_text(ui.fev1, "FEV1: 0.00 L");

    /* FEV6 */
    ui.fev6 = lv_label_create(cont1);
    lv_obj_align(ui.fev6, LV_ALIGN_TOP_LEFT, 5, 80);
    lv_label_set_text(ui.fev6, "FEV6: 0.00 L");

    /* Labels in Container 2 (left side) */
    /* FVC */
    ui.fvc = lv_label_create(cont2);
    lv_obj_align(ui.fvc, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(ui.fvc, "FVC: 0.00 L");

    /* FEV1/FVC */
    ui.fev1_fvc = lv_label_create(cont2);
    lv_obj_align(ui.fev1_fvc, LV_ALIGN_TOP_LEFT, 5, 30);
    lv_label_set_text(ui.fev1_fvc, "FEV1/FVC: 0.0 %");

    /* PEF */
    ui.pef = lv_label_create(cont2);
    lv_obj_align(ui.pef, LV_ALIGN_TOP_LEFT, 5, 55);
    lv_label_set_text(ui.pef, "PEF: 0.00 L/s");

    /* FVL */
    ui.fvl = lv_label_create(cont2);
    lv_obj_align(ui.fvl, LV_ALIGN_TOP_LEFT, 5, 80);
    lv_label_set_text(ui.fvl, "FVL: 0.00");

    /* Create chart for total volume (right side of cont2) */
    ui.chart = lv_chart_create(cont2);
    lv_obj_set_size(ui.chart, 180, 85);
    lv_obj_align(ui.chart, LV_ALIGN_RIGHT_MID, -5, 5);

    /* Set chart type and update mode */
    lv_chart_set_type(ui.chart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(ui.chart, LV_CHART_AXIS_PRIMARY_Y, 0, 500);
    lv_chart_set_point_count(ui.chart, 50);
    lv_chart_set_update_mode(ui.chart, LV_CHART_UPDATE_MODE_SHIFT);

    /* Enable grid lines */
    lv_chart_set_div_line_count(ui.chart, 5, 5);

    /* Style the chart background */
    lv_obj_set_style_bg_color(ui.chart, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_color(ui.chart, lv_color_make(100, 100, 100), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.chart, 1, LV_PART_MAIN);

    /* Style grid lines */
    lv_obj_set_style_line_color(ui.chart, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_line_width(ui.chart, 1, LV_PART_MAIN);

    /* Add a data series with green color */
    ui.chart_series = lv_chart_add_series(ui.chart, lv_color_make(0, 255, 0), LV_CHART_AXIS_PRIMARY_Y);

    /* Style the series line - THIS IS THE KEY PART */
    lv_obj_set_style_line_width(ui.chart, 3, LV_PART_ITEMS);  // Thicker line
    lv_obj_set_style_size(ui.chart, 0, 0, LV_PART_INDICATOR);    // Hide the point markers (circles)
    /* OPTIONAL: Enable circular line drawing for smoother curves (LVGL 9.x) */
    // If you want even smoother curves, uncomment this:
    // lv_obj_set_style_line_rounded(ui.chart, true, LV_PART_ITEMS);

    /* Initialize chart with zeros */
    for(int i = 0; i < 50; i++) {
        lv_chart_set_next_value(ui.chart, ui.chart_series, 0);
    }

    /* Chart title */
    lv_obj_t *chart_title = lv_label_create(cont2);
    lv_label_set_text(chart_title, "Volume (L)");
    lv_obj_align_to(chart_title, ui.chart, LV_ALIGN_OUT_TOP_MID, 0, -2);
    lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_10, 0);

    /* Y-axis label (vertical) */
    lv_obj_t *y_label = lv_label_create(cont2);
    lv_label_set_text(y_label, "L");
    lv_obj_align_to(y_label, ui.chart, LV_ALIGN_OUT_LEFT_MID, -2, 0);
    lv_obj_set_style_text_font(y_label, &lv_font_montserrat_10, 0);

    /* X-axis label */
    lv_obj_t *x_label = lv_label_create(cont2);
    lv_label_set_text(x_label, "Time");
    lv_obj_align_to(x_label, ui.chart, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
    lv_obj_set_style_text_font(x_label, &lv_font_montserrat_10, 0);
}

void ui_update(const spirometry_data_t *data)
{
    if(data == NULL) return;

    char buf[32];

    snprintf(buf, sizeof(buf), "Flow: %.2f L/s", data->flow);
    lv_label_set_text(ui.flow, buf);

    snprintf(buf, sizeof(buf), "Total Volume: %.2f L", data->total_volume);
    lv_label_set_text(ui.tot_vol, buf);

    snprintf(buf, sizeof(buf), "FEV1: %.2f L", data->fev1);
    lv_label_set_text(ui.fev1, buf);

    snprintf(buf, sizeof(buf), "FEV6: %.2f L", data->fev6);
    lv_label_set_text(ui.fev6, buf);

    snprintf(buf, sizeof(buf), "FVC: %.2f L", data->fvc);
    lv_label_set_text(ui.fvc, buf);

    snprintf(buf, sizeof(buf), "FEV1/FVC: %.1f %%", data->fev1_fvc);
    lv_label_set_text(ui.fev1_fvc, buf);

    snprintf(buf, sizeof(buf), "PEF: %.2f L/s", data->pef);
    lv_label_set_text(ui.pef, buf);

    snprintf(buf, sizeof(buf), "FVL: %.2f", data->fvl);
    lv_label_set_text(ui.fvl, buf);

    /* Update chart with total volume data */
    /* Convert float to int (multiply by 10 to preserve one decimal place) */
    int32_t volume_value = (int32_t)(data->total_volume * 10);
    lv_chart_set_next_value(ui.chart, ui.chart_series, volume_value);
    lv_chart_refresh(ui.chart);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
