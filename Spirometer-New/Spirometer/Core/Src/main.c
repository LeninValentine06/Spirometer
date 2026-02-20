<<<<<<< Updated upstream
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
#include "adc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
    float x_prev;   // previous input (flow)
    float y_prev;   // previous output (volume)
} TrapezoidIntegrator;
typedef struct {
    float volume;
    float flow;
} FVLPoint;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SAMPLE_PERIOD_MS   10u
#define SAMPLING_TIME_S    0.01f   // 10 ms
#define EXHALE_THRESHOLD_LPS   0.05f   // detect blowing (~3 LPM)
#define FVL_MAX_POINTS 800   // 8 seconds @ 10ms

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t last_sample_time = 0;
uint32_t raw_value = 0;

float flow_slpm = 0.0f;
float flow_lps  = 0.0f;
float volume_l  = 0.0f;
float flow_offset_slpm = 0.0f;   // auto-calculated zero

uint8_t exhale_active = 0;

uint32_t exhale_start_time = 0;
float exhale_start_volume = 0;

float fev1_l = 0.0f;
float fev6_l = 0.0f;

uint8_t fev1_captured = 0;
uint8_t fev6_captured = 0;

float fvc_l = 0.0f;
float fev1_fvc_ratio = 0.0f;
float pef_lps = 0.0f;

FVLPoint fvl_buffer[FVL_MAX_POINTS];
uint16_t fvl_index = 0;

TrapezoidIntegrator flow_int = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
float adc_to_slpm(uint32_t adc_value);
float trapezoidal_update(TrapezoidIntegrator *i, float x_now);
float calibrate_flow_offset(void);
void update_fev_parameters(float flow_lps, float volume_l, uint32_t now_ms);
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
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  flow_offset_slpm = calibrate_flow_offset();
  last_sample_time =  HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t now = HAL_GetTick();

      if ((now - last_sample_time) >= SAMPLE_PERIOD_MS)
      {
          /* -------- ADC read -------- */
          HAL_ADC_Start(&hadc1);
          HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
          raw_value = HAL_ADC_GetValue(&hadc1);
          HAL_ADC_Stop(&hadc1);

          /* -------- Flow calculation -------- */
          flow_slpm = adc_to_slpm(raw_value);

          /* ---------- offset removal ---------- */
          flow_slpm -= flow_offset_slpm;

          /* ---------- deadband ---------- */
          if (fabsf(flow_slpm) < 1.0f)   // 1 SLPM noise zone
              flow_slpm = 0.0f;

          /* ---------- convert ---------- */
          flow_lps = flow_slpm / 60.0f;

          /* ---------- integrate ---------- */
          volume_l = trapezoidal_update(&flow_int, flow_lps);

          /* compute FEV metrics */
          update_fev_parameters(flow_lps, volume_l, now);

          last_sample_time = now;
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
/* Convert ADC -> Standard Liters Per Minute */
float adc_to_slpm(uint32_t adc_value)
{
    float voltage = (adc_value * 3.3f) / 4095.0f;

    float flow = 51.19f * voltage - 18.94f;

    if (flow < 0.0f)   flow = 0.0f;
    if (flow > 150.0f) flow = 150.0f;

    return flow;
}


/* Trapezoidal integrator update */
float trapezoidal_update(TrapezoidIntegrator *i, float x_now)
{
    float y_now = i->y_prev + (SAMPLING_TIME_S * 0.5f) * (x_now + i->x_prev);

    i->x_prev = x_now;
    i->y_prev = y_now;

    return y_now;
}

float calibrate_flow_offset(void)
{
    float sum = 0.0f;

    for(int i = 0; i < 200; i++)   // 2 seconds @ 10ms
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

        uint32_t raw = HAL_ADC_GetValue(&hadc1);

        HAL_ADC_Stop(&hadc1);

        sum += adc_to_slpm(raw);

        HAL_Delay(10);
    }

    return sum / 200.0f;
}

void update_fev_parameters(float flow_lps, float volume_l, uint32_t now_ms)
{
    static uint32_t low_flow_start = 0;

    /* -------- Detect start of exhalation -------- */
    if (!exhale_active && flow_lps > EXHALE_THRESHOLD_LPS)
    {
    	fvl_index = 0;   // reset FVL capture
        exhale_active = 1;

        exhale_start_time = now_ms;
        exhale_start_volume = volume_l;

        fev1_captured = 0;
        fev6_captured = 0;

        /* reset metrics */
        fvc_l = 0.0f;
        pef_lps = 0.0f;
    }

    /* -------- During exhalation -------- */
    if (exhale_active)
    {
    	/* -------- Store Flow-Volume loop -------- */
    	if (fvl_index < FVL_MAX_POINTS)
    	{
    	    fvl_buffer[fvl_index].volume = volume_l - exhale_start_volume;
    	    fvl_buffer[fvl_index].flow   = flow_lps;
    	    fvl_index++;
    	}

        uint32_t elapsed = now_ms - exhale_start_time;

        float exhaled_volume = volume_l - exhale_start_volume;

        /* -------- FVC (max volume) -------- */
        if (exhaled_volume > fvc_l)
            fvc_l = exhaled_volume;

        /* -------- PEF (max flow) -------- */
        if (flow_lps > pef_lps)
            pef_lps = flow_lps;

        /* -------- FEV1 -------- */
        if (!fev1_captured && elapsed >= 1000)
        {
            fev1_l = exhaled_volume;
            fev1_captured = 1;
        }

        /* -------- FEV6 -------- */
        if (!fev6_captured && elapsed >= 6000)
        {
            fev6_l = exhaled_volume;
            fev6_captured = 1;
        }

        /* -------- End detection with hysteresis -------- */
        if (flow_lps < EXHALE_THRESHOLD_LPS)
        {
            if (low_flow_start == 0)
                low_flow_start = now_ms;

            if ((now_ms - low_flow_start) > 300)
            {
                exhale_active = 0;
                low_flow_start = 0;

                /* compute ratio after exhale ends */
                if (fvc_l > 0.001f)
                    fev1_fvc_ratio = fev1_l / fvc_l;
            }
        }
        else
        {
            low_flow_start = 0;
        }
    }
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
=======
/**
  ******************************************************************************
  * @file    main.c
  * @brief   Main program for RTOS Spirometry System
  * @author  Research-grade implementation
  ******************************************************************************
  */

#include "main.h"
#include "adc.h"
#include "gpio.h"
#include "spirometry_tasks.h"
#include "metrics_collector.h"
#include <stdio.h>

/* Private variables */
extern TIM_HandleTypeDef htim2;  // Hardware timestamp timer (declared in stm32f4xx_hal_timebase_tim.c)
TIM_HandleTypeDef htim5;  // Runtime stats timer (1 MHz, 32-bit)

/* Private function prototypes */
void SystemClock_Config(void);
static void MX_TIM2_Init(void);
static void MX_TIM5_Init(void);
static void DWT_Init(void);

/* Printf redirection for ITM/SWO */
#ifdef USE_ITM_TRACE
int _write(int file, char *ptr, int len)
{
    for(int i = 0; i < len; i++)
    {
        ITM_SendChar((*ptr++));
    }
    return len;
}
#endif

/**
  * @brief  The application entry point.
  */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();   // Hardware timestamp timer
    MX_TIM5_Init();   // Runtime stats timer

    /* Initialize DWT cycle counter for WCET measurement */
    DWT_Init();

    /* Enable ITM trace if configured */
    #ifdef USE_ITM_TRACE
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR = 0xC5ACCE55;
    ITM->TER = 0x1;
    ITM->TCR = ITM_TCR_ITMENA_Msk;
    #endif

    /* Start hardware timers */
    HAL_TIM_Base_Start(&htim2);  // Start timestamp timer
    HAL_TIM_Base_Start(&htim5);  // Start runtime stats timer

    /* Print banner */
    printf("\r\n\r\n");
    printf("************************************************************************\r\n");
    printf("*                                                                      *\r\n");
    printf("*        RTOS SPIROMETRY SYSTEM - RESEARCH GRADE                      *\r\n");
    printf("*        STM32F4 + FreeRTOS v10.4.6                                   *\r\n");
    printf("*                                                                      *\r\n");
    printf("************************************************************************\r\n");
    printf("\r\n");
    printf("System Configuration:\r\n");
    printf("  CPU:              %lu MHz\r\n", SystemCoreClock / 1000000);
    printf("  Sampling Rate:    %u Hz (%u ms period)\r\n",
           SAMPLING_FREQUENCY_HZ, SAMPLING_PERIOD_MS);
    printf("  FreeRTOS Tick:    %lu Hz\r\n", configTICK_RATE_HZ);
    printf("  Total Heap:       %u bytes\r\n", (unsigned int)configTOTAL_HEAP_SIZE);
    printf("\r\n");

    /* Initialize metrics system */
    Metrics_Init();

    /* Calibrate flow sensor */
    extern float flow_offset_slpm;
    flow_offset_slpm = Spirometry_CalibrateFlowOffset();

    /* Initialize RTOS objects and create tasks */
    Spirometry_InitRTOS();

    /* Start the scheduler */
    printf("Starting FreeRTOS scheduler...\r\n\r\n");
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    printf("ERROR: Scheduler returned!\r\n");
    Error_Handler();

    /* Infinite loop */
    while (1)
    {
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage */
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

    /** Initializes the CPU, AHB and APB buses clocks */
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

/**
  * @brief TIM2 Initialization Function (Hardware Timestamp Timer)
  * @note  Configured as 32-bit timer at 1 MHz (1 tick = 1 microsecond)
  */
static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = (SystemCoreClock / 1000000) - 1;  // 1 MHz timer
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFFFFF;  // 32-bit counter
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief TIM5 Initialization Function (Runtime Stats Timer)
  * @note  Configured as 32-bit timer at 1 MHz for FreeRTOS runtime stats
  */
static void MX_TIM5_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = (SystemCoreClock / 1000000) - 1;  // 1 MHz timer
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 0xFFFFFFFF;  // 32-bit counter
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief Initialize DWT cycle counter for WCET measurement
  */
static void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // Enable trace
    DWT->CYCCNT = 0;                                  // Reset cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;             // Enable cycle counter
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called when TIM2 interrupt took place, inside
  *         HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  *         a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        HAL_IncTick();
    }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    __disable_irq();
    printf("\r\n!!! ERROR HANDLER CALLED !!!\r\n");
    while (1)
    {
    }
}

/* ============================================================================
   FREERTOS HOOK FUNCTIONS
   ============================================================================ */

/**
  * @brief  Stack overflow hook
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("\r\n!!! STACK OVERFLOW IN TASK: %s !!!\r\n", pcTaskName);

    __disable_irq();
    while(1);
}

/**
  * @brief  Malloc failed hook
  */
void vApplicationMallocFailedHook(void)
{
    printf("\r\n!!! HEAP ALLOCATION FAILED !!!\r\n");
    printf("Free heap: %u bytes\r\n", (unsigned int)xPortGetFreeHeapSize());

    __disable_irq();
    while(1);
}

/* ============================================================================
   HAL MSP INITIALIZATION
   ============================================================================ */

/**
  * @brief TIM_Base MSP Initialization
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_ENABLE();
    }
    else if(htim_base->Instance == TIM5)
    {
        __HAL_RCC_TIM5_CLK_ENABLE();
    }
}

/**
  * @brief TIM_Base MSP De-Initialization
  */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
    if(htim_base->Instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_DISABLE();
    }
    else if(htim_base->Instance == TIM5)
    {
        __HAL_RCC_TIM5_CLK_DISABLE();
    }
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
    printf("Assert failed: file %s line %lu\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */
>>>>>>> Stashed changes
