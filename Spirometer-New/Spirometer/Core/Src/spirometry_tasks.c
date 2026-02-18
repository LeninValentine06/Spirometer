/**
  ******************************************************************************
  * @file    spirometry_tasks.c
  * @brief   RTOS task implementations for spirometry system
  ******************************************************************************
  */

#include "spirometry_tasks.h"
#include "metrics_collector.h"
#include "main.h"
#include "adc.h"
#include "gpio.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/* ============================================================================
   EXTERNAL PERIPHERALS
   ============================================================================ */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

/* ============================================================================
   RTOS HANDLES
   ============================================================================ */

TaskHandle_t acquisitionTaskHandle = NULL;
TaskHandle_t processingTaskHandle = NULL;
TaskHandle_t analysisTaskHandle = NULL;
TaskHandle_t metricsTaskHandle = NULL;

QueueHandle_t rawSampleQueue = NULL;
QueueHandle_t processedSampleQueue = NULL;
QueueHandle_t resultsQueue = NULL;

/* ============================================================================
   PROCESSING STATE (Local to tasks)
   ============================================================================ */

static IntegratorState volume_integrator = {0};
float flow_offset_slpm = 0.0f;  // Exported for main.c calibration
static BreathAnalyzer breath_analyzer = {0};
static uint32_t sequence_counter = 0;

/* ============================================================================
   HELPER FUNCTIONS
   ============================================================================ */

/* ============================================================================
   HELPER FUNCTIONS
   ============================================================================ */

/**
  * @brief Get hardware timestamp in microseconds
  */
uint32_t GetHardwareTimestamp_us(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/**
  * @brief Get CPU cycle count
  */
uint32_t GetCycleCount(void)
{
    return DWT->CYCCNT;
}

/**
  * @brief Convert cycles to microseconds
  */
float CyclesToMicroseconds(uint32_t cycles)
{
    return (float)cycles / (SystemCoreClock / 1000000.0f);
}

/**
  * @brief Convert ADC value to flow in SLPM
  */
static float ADC_to_SLPM(uint32_t adc_value)
{
    float voltage = (adc_value * 3.3f) / 4095.0f;
    float flow = 51.19f * voltage - 18.94f;

    // Clamp to physical limits
    if (flow < 0.0f) flow = 0.0f;
    if (flow > 150.0f) flow = 150.0f;

    return flow;
}

/**
  * @brief Trapezoidal integration update
  */
static float Trapezoidal_Update(IntegratorState *state, float x_now, float dt)
{
    float y_now = state->y_prev + (dt * 0.5f) * (x_now + state->x_prev);

    state->x_prev = x_now;
    state->y_prev = y_now;

    return y_now;
}

/* ============================================================================
   TASK 1: ACQUISITION (Highest Priority, Periodic 10ms)
   ============================================================================ */

void Task_Acquisition(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(SAMPLING_PERIOD_MS);

    RawSample sample;
    uint32_t expected_release_us = 0;

    xLastWakeTime = xTaskGetTickCount();

    printf("Acquisition Task Started (Priority: %d)\r\n",
           (int)uxTaskPriorityGet(NULL));

    for (;;)
    {
        /* ========== WCET MEASUREMENT START ========== */
        uint32_t task_start_cycles = GetCycleCount();

        #if ENABLE_GPIO_PROFILING
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        #endif

        /* ========== CRITICAL SECTION: ADC ACQUISITION ========== */

        // Get hardware timestamp IMMEDIATELY
        uint32_t hw_timestamp_us = GetHardwareTimestamp_us();

        // ADC read with timeout
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 1) != HAL_OK) {
            // ADC timeout - mark as fault
            sample.adc_raw = 0xFFFFFFFF;
            g_Metrics.integrity.adc_faults++;
        } else {
            sample.adc_raw = HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);

        // Populate sample metadata
        sample.hw_timestamp_us = hw_timestamp_us;
        sample.sequence_number = sequence_counter++;

        // Post to processing queue (non-blocking)
        if (xQueueSend(rawSampleQueue, &sample, 0) != pdTRUE) {
            // Queue overflow - data loss
            g_Metrics.integrity.samples_lost_overflow++;
            g_Metrics.safety.queue_overflow = 1;
        }

        // Update integrity counter
        g_Metrics.integrity.samples_acquired++;

        /* ========== WCET MEASUREMENT END ========== */
        uint32_t task_end_cycles = GetCycleCount();
        uint32_t execution_cycles = task_end_cycles - task_start_cycles;

        Metrics_UpdateWCET(&g_Metrics.wcet_acquisition, execution_cycles);

        /* ========== DEADLINE CHECK ========== */
        uint32_t completion_us = GetHardwareTimestamp_us();
        Metrics_UpdateDeadline(expected_release_us, completion_us, SAMPLING_PERIOD_MS * 1000);

        expected_release_us += (SAMPLING_PERIOD_MS * 1000);

        #if ENABLE_GPIO_PROFILING
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        #endif

        /* Sleep until next period */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* ============================================================================
   TASK 2: PROCESSING (Event-driven)
   ============================================================================ */

void Task_Processing(void *pvParameters)
{
    RawSample raw;
    ProcessedSample processed;
    uint32_t expected_sequence = 0;

    printf("Processing Task Started (Priority: %d)\r\n",
           (int)uxTaskPriorityGet(NULL));

    for (;;)
    {
        // Block waiting for new sample
        if (xQueueReceive(rawSampleQueue, &raw, portMAX_DELAY) == pdTRUE)
        {
            /* ========== WCET MEASUREMENT START ========== */
            uint32_t task_start_cycles = GetCycleCount();

            #if ENABLE_GPIO_PROFILING
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
            #endif

            /* ========== SEQUENCE GAP DETECTION ========== */
            if (raw.sequence_number != expected_sequence) {
                uint32_t gap = raw.sequence_number - expected_sequence;
                g_Metrics.integrity.sequence_gaps_detected += gap;
            }
            expected_sequence = raw.sequence_number + 1;

            /* ========== FAULT HANDLING ========== */
            if (raw.adc_raw == 0xFFFFFFFF) {
                // Invalid sample - skip processing
                #if ENABLE_GPIO_PROFILING
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
                #endif
                continue;
            }

            /* ========== SIGNAL PROCESSING CHAIN ========== */

            // 1. ADC to physical units
            float flow_slpm = ADC_to_SLPM(raw.adc_raw);

            // 2. Apply calibration offset
            flow_slpm -= flow_offset_slpm;

            // 3. Apply deadband (noise rejection)
            if (fabsf(flow_slpm) < FLOW_DEADBAND_SLPM) {
                flow_slpm = 0.0f;
            }

            // 4. Convert to L/s
            processed.flow_lps = flow_slpm / 60.0f;

            // 5. Integrate volume using trapezoidal method
            processed.volume_l = Trapezoidal_Update(&volume_integrator,
                                                      processed.flow_lps,
                                                      SAMPLING_PERIOD_S);

            // 6. Preserve metadata
            processed.hw_timestamp_us = raw.hw_timestamp_us;
            processed.sequence_number = raw.sequence_number;
            processed.processing_start_us = GetHardwareTimestamp_us();

            /* ========== SAFETY CHECKS ========== */
            Metrics_CheckSafety(&processed);

            /* ========== POST TO ANALYSIS TASK ========== */
            if (xQueueSend(processedSampleQueue, &processed, 0) != pdTRUE) {
                g_Metrics.integrity.samples_lost_overflow++;
            }

            g_Metrics.integrity.samples_processed++;

            /* ========== JITTER MEASUREMENT ========== */
            Metrics_UpdateJitter(raw.hw_timestamp_us, SAMPLING_PERIOD_MS * 1000);

            /* ========== WCET MEASUREMENT END ========== */
            uint32_t task_end_cycles = GetCycleCount();
            uint32_t execution_cycles = task_end_cycles - task_start_cycles;

            Metrics_UpdateWCET(&g_Metrics.wcet_processing, execution_cycles);

            #if ENABLE_GPIO_PROFILING
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
            #endif
        }
    }
}

/* ============================================================================
   TASK 3: ANALYSIS (Event-driven)
   ============================================================================ */

void Task_Analysis(void *pvParameters)
{
    ProcessedSample sample;

    printf("Analysis Task Started (Priority: %d)\r\n",
           (int)uxTaskPriorityGet(NULL));

    for (;;)
    {
        if (xQueueReceive(processedSampleQueue, &sample, portMAX_DELAY) == pdTRUE)
        {
            /* ========== WCET MEASUREMENT START ========== */
            uint32_t task_start_cycles = GetCycleCount();

            #if ENABLE_GPIO_PROFILING
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
            #endif

            /* ========== END-TO-END LATENCY ========== */
            uint32_t current_time_us = GetHardwareTimestamp_us();
            Metrics_UpdateLatency(sample.hw_timestamp_us, current_time_us);

            /* ========== BREATH STATE MACHINE ========== */

            switch (breath_analyzer.phase)
            {
                case BREATH_IDLE:
                {
                    // Detect exhalation start
                    if (sample.flow_lps > EXHALE_THRESHOLD_LPS) {
                        breath_analyzer.phase = BREATH_EXHALE;
                        breath_analyzer.breath_id++;
                        breath_analyzer.phase_start_time_us = sample.hw_timestamp_us;
                        breath_analyzer.volume_at_phase_start = sample.volume_l;

                        // Reset breath metrics
                        breath_analyzer.fvc_l = 0.0f;
                        breath_analyzer.fev1_l = 0.0f;
                        breath_analyzer.fev6_l = 0.0f;
                        breath_analyzer.pef_lps = 0.0f;
                        breath_analyzer.fev1_captured = 0;
                        breath_analyzer.fev6_captured = 0;
                        breath_analyzer.fvl_count = 0;
                        breath_analyzer.low_flow_start_us = 0;

                        printf("\r\n>>> EXHALATION STARTED (Breath #%lu) <<<\r\n",
                               breath_analyzer.breath_id);
                    }
                    break;
                }

                case BREATH_EXHALE:
                {
                    uint32_t elapsed_us = sample.hw_timestamp_us - breath_analyzer.phase_start_time_us;
                    float exhaled_vol = sample.volume_l - breath_analyzer.volume_at_phase_start;

                    // Update FVC (maximum exhaled volume)
                    if (exhaled_vol > breath_analyzer.fvc_l) {
                        breath_analyzer.fvc_l = exhaled_vol;
                    }

                    // Update PEF (peak expiratory flow)
                    if (sample.flow_lps > breath_analyzer.pef_lps) {
                        breath_analyzer.pef_lps = sample.flow_lps;
                    }

                    // Capture FEV1 at 1 second
                    if (!breath_analyzer.fev1_captured && elapsed_us >= 1000000) {
                        breath_analyzer.fev1_l = exhaled_vol;
                        breath_analyzer.fev1_captured = 1;
                        printf(">>> FEV1 captured: %.3f L\r\n", breath_analyzer.fev1_l);
                    }

                    // Capture FEV6 at 6 seconds
                    if (!breath_analyzer.fev6_captured && elapsed_us >= 6000000) {
                        breath_analyzer.fev6_l = exhaled_vol;
                        breath_analyzer.fev6_captured = 1;
                        printf(">>> FEV6 captured: %.3f L\r\n", breath_analyzer.fev6_l);
                    }

                    // Store F-V loop point
                    if (breath_analyzer.fvl_count < FVL_MAX_POINTS) {
                        breath_analyzer.fvl_buffer[breath_analyzer.fvl_count].volume = exhaled_vol;
                        breath_analyzer.fvl_buffer[breath_analyzer.fvl_count].flow = sample.flow_lps;
                        breath_analyzer.fvl_count++;
                    }

                    // Detect exhalation end with hysteresis
                    if (sample.flow_lps < EXHALE_THRESHOLD_LPS) {
                        if (breath_analyzer.low_flow_start_us == 0) {
                            breath_analyzer.low_flow_start_us = sample.hw_timestamp_us;
                        }

                        uint32_t low_flow_duration = sample.hw_timestamp_us - breath_analyzer.low_flow_start_us;

                        if (low_flow_duration > 300000) {  // 300ms hysteresis
                            breath_analyzer.phase = BREATH_COMPLETE;
                        }
                    } else {
                        breath_analyzer.low_flow_start_us = 0;
                    }

                    break;
                }

                case BREATH_COMPLETE:
                {
                    // Compute final metrics
                    SpirometryResult result;
                    memset(&result, 0, sizeof(result));

                    result.fvc_l = breath_analyzer.fvc_l;
                    result.fev1_l = breath_analyzer.fev1_l;
                    result.fev6_l = breath_analyzer.fev6_l;
                    result.pef_lps = breath_analyzer.pef_lps;

                    if (breath_analyzer.fvc_l > 0.001f) {
                        result.fev1_fvc_ratio = breath_analyzer.fev1_l / breath_analyzer.fvc_l;
                    }

                    result.breath_id = breath_analyzer.breath_id;
                    result.breath_complete = 1;
                    result.fvl_count = breath_analyzer.fvl_count;

                    // Copy F-V loop data
                    memcpy(result.fvl_data,
                           breath_analyzer.fvl_buffer,
                           breath_analyzer.fvl_count * sizeof(FVLPoint));

                    // Post result
                    xQueueSend(resultsQueue, &result, 0);

                    printf("\r\n>>> EXHALATION COMPLETE <<<\r\n");
                    printf("    FVC:       %.3f L\r\n", result.fvc_l);
                    printf("    FEV1:      %.3f L\r\n", result.fev1_l);
                    printf("    FEV1/FVC:  %.2f%%\r\n", result.fev1_fvc_ratio * 100.0f);
                    printf("    PEF:       %.2f L/s\r\n", result.pef_lps);
                    printf("    F-V Points: %u\r\n\r\n", result.fvl_count);

                    // Reset to idle
                    breath_analyzer.phase = BREATH_IDLE;

                    break;
                }
            }

            g_Metrics.integrity.samples_analyzed++;

            /* ========== WCET MEASUREMENT END ========== */
            uint32_t task_end_cycles = GetCycleCount();
            uint32_t execution_cycles = task_end_cycles - task_start_cycles;

            Metrics_UpdateWCET(&g_Metrics.wcet_analysis, execution_cycles);

            #if ENABLE_GPIO_PROFILING
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
            #endif
        }
    }
}

/* ============================================================================
   INITIALIZATION FUNCTIONS
   ============================================================================ */

void Spirometry_InitRTOS(void)
{
    printf("\r\n");
    printf("==========================================\r\n");
    printf("  RTOS SPIROMETRY SYSTEM INITIALIZATION\r\n");
    printf("==========================================\r\n");

    // Create queues
    rawSampleQueue = xQueueCreate(RAW_SAMPLE_QUEUE_SIZE, sizeof(RawSample));
    processedSampleQueue = xQueueCreate(PROCESSED_QUEUE_SIZE, sizeof(ProcessedSample));
    resultsQueue = xQueueCreate(RESULTS_QUEUE_SIZE, sizeof(SpirometryResult));

    if (!rawSampleQueue || !processedSampleQueue || !resultsQueue) {
        printf("ERROR: Queue creation failed!\r\n");
        Error_Handler();
    }

    printf("Queues created successfully\r\n");

    // Create tasks
    BaseType_t result;

    result = xTaskCreate(Task_Acquisition,
                         "Acquisition",
                         ACQUISITION_STACK_SIZE,
                         NULL,
                         ACQUISITION_TASK_PRIORITY,
                         &acquisitionTaskHandle);
    if (result != pdPASS) {
        printf("ERROR: Acquisition task creation failed!\r\n");
        Error_Handler();
    }

    result = xTaskCreate(Task_Processing,
                         "Processing",
                         PROCESSING_STACK_SIZE,
                         NULL,
                         PROCESSING_TASK_PRIORITY,
                         &processingTaskHandle);
    if (result != pdPASS) {
        printf("ERROR: Processing task creation failed!\r\n");
        Error_Handler();
    }

    result = xTaskCreate(Task_Analysis,
                         "Analysis",
                         ANALYSIS_STACK_SIZE,
                         NULL,
                         ANALYSIS_TASK_PRIORITY,
                         &analysisTaskHandle);
    if (result != pdPASS) {
        printf("ERROR: Analysis task creation failed!\r\n");
        Error_Handler();
    }

    result = xTaskCreate(Task_Metrics,
                         "Metrics",
                         METRICS_STACK_SIZE,
                         NULL,
                         METRICS_TASK_PRIORITY,
                         &metricsTaskHandle);
    if (result != pdPASS) {
        printf("ERROR: Metrics task creation failed!\r\n");
        Error_Handler();
    }

    printf("Tasks created successfully\r\n");
    printf("System ready to start scheduler\r\n");
    printf("==========================================\r\n\r\n");
}

float Spirometry_CalibrateFlowOffset(void)
{
    float sum = 0.0f;
    const uint32_t samples = 200;

    printf("Calibrating flow sensor offset (%lu samples)...\r\n", samples);

    for (uint32_t i = 0; i < samples; i++)
    {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);

        sum += ADC_to_SLPM(raw);

        HAL_Delay(10);
    }

    float offset = sum / samples;
    printf("Calibration complete. Offset: %.2f SLPM\r\n\r\n", offset);

    return offset;
}
