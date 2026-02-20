/**
  ******************************************************************************
  * @file    spirometry_tasks.h
  * @brief   RTOS task definitions for spirometry system
  ******************************************************************************
  */

#ifndef __SPIROMETRY_TASKS_H
#define __SPIROMETRY_TASKS_H

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "spirometry_types.h"

/* ============================================================================
   TASK PRIORITIES
   ============================================================================ */

#define ACQUISITION_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)  // Highest
#define PROCESSING_TASK_PRIORITY    (tskIDLE_PRIORITY + 2)
#define ANALYSIS_TASK_PRIORITY      (tskIDLE_PRIORITY + 1)
#define METRICS_TASK_PRIORITY       (tskIDLE_PRIORITY + 0)  // Lowest

/* ============================================================================
   STACK SIZES (in words, will be multiplied by 4 for bytes)
   ============================================================================ */

#define ACQUISITION_STACK_SIZE      128
#define PROCESSING_STACK_SIZE       256
#define ANALYSIS_STACK_SIZE         384
#define METRICS_STACK_SIZE          384

/* ============================================================================
   QUEUE SIZES
   ============================================================================ */

#define RAW_SAMPLE_QUEUE_SIZE       10
#define PROCESSED_QUEUE_SIZE        10
#define RESULTS_QUEUE_SIZE          5

/* ============================================================================
   TASK HANDLES (extern declarations)
   ============================================================================ */

extern TaskHandle_t acquisitionTaskHandle;
extern TaskHandle_t processingTaskHandle;
extern TaskHandle_t analysisTaskHandle;
extern TaskHandle_t metricsTaskHandle;

extern QueueHandle_t rawSampleQueue;
extern QueueHandle_t processedSampleQueue;
extern QueueHandle_t resultsQueue;

/* ============================================================================
   GLOBAL VARIABLES (for calibration)
   ============================================================================ */

extern float flow_offset_slpm;  // Flow sensor calibration offset

/* ============================================================================
   TASK FUNCTIONS
   ============================================================================ */

/**
  * @brief Task 1: ADC Acquisition (Highest Priority, Periodic 10ms)
  */
void Task_Acquisition(void *pvParameters);

/**
  * @brief Task 2: Signal Processing (Event-driven)
  */
void Task_Processing(void *pvParameters);

/**
  * @brief Task 3: Spirometry Analysis (Event-driven)
  */
void Task_Analysis(void *pvParameters);

/**
  * @brief Task 4: Metrics Collection and Reporting (Periodic 5s)
  */
void Task_Metrics(void *pvParameters);

/* ============================================================================
   INITIALIZATION
   ============================================================================ */

/**
  * @brief Initialize all RTOS objects (queues, tasks)
  */
void Spirometry_InitRTOS(void);

/**
  * @brief Calibrate flow sensor offset
  */
float Spirometry_CalibrateFlowOffset(void);

/* ============================================================================
   UTILITY FUNCTIONS
   ============================================================================ */

/**
  * @brief Get hardware timestamp in microseconds
  */
uint32_t GetHardwareTimestamp_us(void);

/**
  * @brief Get CPU cycle count
  */
uint32_t GetCycleCount(void);

/**
  * @brief Convert cycles to microseconds
  */
float CyclesToMicroseconds(uint32_t cycles);

#endif /* __SPIROMETRY_TASKS_H */
