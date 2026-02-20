/**
  ******************************************************************************
  * @file    metrics_collector.c
  * @brief   Performance metrics collection implementation
  ******************************************************************************
  */

#include "metrics_collector.h"
#include "spirometry_tasks.h"
#include "main.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

/* ============================================================================
   EXTERNAL DEPENDENCIES
   ============================================================================ */

extern TaskHandle_t acquisitionTaskHandle;
extern TaskHandle_t processingTaskHandle;
extern TaskHandle_t analysisTaskHandle;
extern TaskHandle_t metricsTaskHandle;

extern QueueHandle_t rawSampleQueue;
extern QueueHandle_t processedSampleQueue;
extern QueueHandle_t resultsQueue;

extern TIM_HandleTypeDef htim5;  // Runtime stats timer

/* ============================================================================
   GLOBAL METRICS
   ============================================================================ */

SystemMetrics g_Metrics = {0};

/* ============================================================================
   RUNTIME STATS TIMER CONFIGURATION
   ============================================================================ */

void ConfigureTimerForRuntimeStats(void)
{
    // TIM5 is configured as a 32-bit timer at 1 MHz (1 tick = 1 microsecond)
    // This is done in main.c during peripheral initialization
}

uint32_t GetRuntimeCounterValue(void)
{
    return __HAL_TIM_GET_COUNTER(&htim5);
}

/* ============================================================================
   METRICS UPDATE FUNCTIONS
   ============================================================================ */

void Metrics_UpdateLatency(uint32_t trigger_time_us, uint32_t completion_time_us)
{
    // Handle timer wraparound (32-bit timer)
    uint32_t latency_us;

    if (completion_time_us >= trigger_time_us) {
        latency_us = completion_time_us - trigger_time_us;
    } else {
        // Timer wrapped around
        latency_us = (0xFFFFFFFF - trigger_time_us) + completion_time_us;
    }

    // Update statistics
    g_Metrics.timing.e2e_latency_sum_us += latency_us;
    g_Metrics.timing.latency_sample_count++;

    // Update max
    if (latency_us > g_Metrics.timing.e2e_latency_max_us) {
        g_Metrics.timing.e2e_latency_max_us = latency_us;
    }

    // Update min
    if (g_Metrics.timing.latency_sample_count == 1 ||
        latency_us < g_Metrics.timing.e2e_latency_min_us) {
        g_Metrics.timing.e2e_latency_min_us = latency_us;
    }

    // Calculate average
    g_Metrics.timing.e2e_latency_avg_us =
        g_Metrics.timing.e2e_latency_sum_us / g_Metrics.timing.latency_sample_count;
}

void Metrics_UpdateJitter(uint32_t arrival_time_us, uint32_t nominal_period_us)
{
    if (g_Metrics.timing.last_arrival_time_us != 0) {
        // Calculate actual inter-arrival interval
        uint32_t actual_interval_us;

        if (arrival_time_us >= g_Metrics.timing.last_arrival_time_us) {
            actual_interval_us = arrival_time_us - g_Metrics.timing.last_arrival_time_us;
        } else {
            // Timer wraparound
            actual_interval_us = (0xFFFFFFFF - g_Metrics.timing.last_arrival_time_us) + arrival_time_us;
        }

        // Calculate jitter (absolute deviation from nominal period)
        int32_t deviation = (int32_t)actual_interval_us - (int32_t)nominal_period_us;
        float jitter_us = fabsf((float)deviation);

        // Update statistics
        g_Metrics.timing.jitter_sum_us += jitter_us;
        g_Metrics.timing.jitter_sample_count++;

        // Update max
        if (jitter_us > g_Metrics.timing.jitter_max_us) {
            g_Metrics.timing.jitter_max_us = jitter_us;
        }

        // Calculate average
        g_Metrics.timing.jitter_avg_us =
            g_Metrics.timing.jitter_sum_us / g_Metrics.timing.jitter_sample_count;
    }

    g_Metrics.timing.last_arrival_time_us = arrival_time_us;
}

void Metrics_UpdateDeadline(uint32_t release_time_us, uint32_t completion_time_us, uint32_t deadline_us)
{
    uint32_t absolute_deadline_us = release_time_us + deadline_us;

    g_Metrics.timing.total_samples++;

    // Check for deadline miss
    // Handle wraparound carefully
    int32_t slack = (int32_t)(absolute_deadline_us - completion_time_us);

    if (slack < 0) {
        // Deadline missed
        g_Metrics.timing.deadline_misses++;
    }

    // Calculate miss rate
    g_Metrics.timing.deadline_miss_rate_percent =
        ((float)g_Metrics.timing.deadline_misses / g_Metrics.timing.total_samples) * 100.0f;
}

void Metrics_UpdateWCET(WCETMetrics *wcet, uint32_t execution_cycles)
{
    wcet->current_execution_cycles = execution_cycles;
    wcet->current_execution_us = CyclesToMicroseconds(execution_cycles);
    wcet->execution_count++;

    // Update WCET if this is a new maximum
    if (execution_cycles > wcet->acquisition_wcet_cycles) {
        wcet->acquisition_wcet_cycles = execution_cycles;
        wcet->acquisition_wcet_us = wcet->current_execution_us;
    }
}

void Metrics_CollectCPU(void)
{
    // Use FreeRTOS runtime stats
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize;
    uint32_t ulTotalRunTime;

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

    if (pxTaskStatusArray != NULL) {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

        // Calculate CPU utilization
        // Total runtime counter is in microseconds (from TIM5)
        // System uptime in microseconds
        uint32_t uptime_us = ulTotalRunTime;

        // Sum of all task runtimes
        uint32_t total_task_runtime = 0;
        for (UBaseType_t x = 0; x < uxArraySize; x++) {
            total_task_runtime += pxTaskStatusArray[x].ulRunTimeCounter;
        }

        // CPU utilization = (total_task_runtime / uptime) * 100
        if (ulTotalRunTime > 0) {
            g_Metrics.resources.cpu_utilization_percent =
                ((float)total_task_runtime / ulTotalRunTime) * 100.0f;

            // Update peak
            if (g_Metrics.resources.cpu_utilization_percent >
                g_Metrics.resources.cpu_utilization_peak_percent) {
                g_Metrics.resources.cpu_utilization_peak_percent =
                    g_Metrics.resources.cpu_utilization_percent;
            }
        }

        vPortFree(pxTaskStatusArray);
    }
}

void Metrics_CollectHeap(void)
{
    g_Metrics.resources.heap_free_bytes = xPortGetFreeHeapSize();
    g_Metrics.resources.heap_min_free_bytes = xPortGetMinimumEverFreeHeapSize();
    g_Metrics.resources.heap_used_bytes = configTOTAL_HEAP_SIZE - g_Metrics.resources.heap_free_bytes;
}

void Metrics_CollectStack(void)
{
    if (acquisitionTaskHandle) {
        g_Metrics.resources.acquisition_stack_hwm =
            uxTaskGetStackHighWaterMark(acquisitionTaskHandle) * 4;  // Words to bytes
    }

    if (processingTaskHandle) {
        g_Metrics.resources.processing_stack_hwm =
            uxTaskGetStackHighWaterMark(processingTaskHandle) * 4;
    }

    if (analysisTaskHandle) {
        g_Metrics.resources.analysis_stack_hwm =
            uxTaskGetStackHighWaterMark(analysisTaskHandle) * 4;
    }

    if (metricsTaskHandle) {
        g_Metrics.resources.metrics_stack_hwm =
            uxTaskGetStackHighWaterMark(metricsTaskHandle) * 4;
    }

    // Check for stack overflow risk
    const uint32_t STACK_WARNING_THRESHOLD = 64;  // bytes

    if (g_Metrics.resources.acquisition_stack_hwm < STACK_WARNING_THRESHOLD ||
        g_Metrics.resources.processing_stack_hwm < STACK_WARNING_THRESHOLD ||
        g_Metrics.resources.analysis_stack_hwm < STACK_WARNING_THRESHOLD ||
        g_Metrics.resources.metrics_stack_hwm < STACK_WARNING_THRESHOLD) {
        g_Metrics.safety.stack_overflow_risk = 1;
    }
}

void Metrics_CollectQueues(void)
{
    if (rawSampleQueue) {
        UBaseType_t items = uxQueueMessagesWaiting(rawSampleQueue);
        if (items > g_Metrics.resources.raw_queue_peak_usage) {
            g_Metrics.resources.raw_queue_peak_usage = items;
        }
    }

    if (processedSampleQueue) {
        UBaseType_t items = uxQueueMessagesWaiting(processedSampleQueue);
        if (items > g_Metrics.resources.processed_queue_peak_usage) {
            g_Metrics.resources.processed_queue_peak_usage = items;
        }
    }

    if (resultsQueue) {
        UBaseType_t items = uxQueueMessagesWaiting(resultsQueue);
        if (items > g_Metrics.resources.results_queue_peak_usage) {
            g_Metrics.resources.results_queue_peak_usage = items;
        }
    }
}

void Metrics_CheckSafety(ProcessedSample *sample)
{
    // Reset safety flags each check
    g_Metrics.safety.implausible_flow = 0;
    g_Metrics.safety.negative_volume = 0;
    g_Metrics.safety.integrator_overflow = 0;

    // Check for implausible flow (> 15 L/s is physiologically impossible)
    if (fabsf(sample->flow_lps) > 15.0f) {
        g_Metrics.safety.implausible_flow = 1;
        g_Metrics.integrity.plausibility_violations++;

        // Clamp to safe range
        sample->flow_lps = (sample->flow_lps > 0) ? 15.0f : -15.0f;
    }

    // Check for negative volume (with small tolerance for noise)
    if (sample->volume_l < -0.1f) {
        g_Metrics.safety.negative_volume = 1;
        g_Metrics.integrity.plausibility_violations++;

        // Reset integrator
        Metrics_ResetIntegrator();
    }

    // Check for integrator overflow (> 10 L is implausible)
    if (sample->volume_l > 10.0f) {
        g_Metrics.safety.integrator_overflow = 1;
        g_Metrics.integrity.plausibility_violations++;

        // Clamp and warn
        sample->volume_l = 10.0f;
    }
}

void Metrics_ResetIntegrator(void)
{
    // This would need access to the integrator state
    // In a real implementation, you'd use a function pointer or message
    // For now, we just log the event
    printf(">>> INTEGRATOR RESET (Safety violation) <<<\r\n");
}

void Metrics_Init(void)
{
    memset(&g_Metrics, 0, sizeof(SystemMetrics));
    printf("Metrics system initialized\r\n");
}

/* ============================================================================
   REPORTING FUNCTIONS
   ============================================================================ */

void Metrics_PrintReport(void)
{
    printf("\r\n");
    printf("========================================================================\r\n");
    printf("  REAL-TIME PERFORMANCE METRICS REPORT\r\n");
    printf("  Uptime: %lu seconds\r\n", g_Metrics.system_uptime_seconds);
    printf("========================================================================\r\n");

    /* ===== TIMING METRICS ===== */
    printf("\r\n[1] TIMING METRICS\r\n");
    printf("--------------------------------------------------------------------\r\n");
    printf("End-to-End Latency (Sample to Result):\r\n");
    printf("  Average:  %7.2f us\r\n", g_Metrics.timing.e2e_latency_avg_us);
    printf("  Minimum:  %7.2f us\r\n", g_Metrics.timing.e2e_latency_min_us);
    printf("  Maximum:  %7.2f us\r\n", g_Metrics.timing.e2e_latency_max_us);
    printf("  Samples:  %lu\r\n", g_Metrics.timing.latency_sample_count);

    printf("\r\nSampling Jitter (Inter-arrival variation):\r\n");
    printf("  Average:  %7.2f us\r\n", g_Metrics.timing.jitter_avg_us);
    printf("  Maximum:  %7.2f us\r\n", g_Metrics.timing.jitter_max_us);
    printf("  Samples:  %lu\r\n", g_Metrics.timing.jitter_sample_count);

    printf("\r\nDeadline Performance:\r\n");
    printf("  Total Samples:    %lu\r\n", g_Metrics.timing.total_samples);
    printf("  Deadline Misses:  %lu\r\n", g_Metrics.timing.deadline_misses);
    printf("  Miss Rate:        %.4f%%\r\n", g_Metrics.timing.deadline_miss_rate_percent);

    /* ===== WCET METRICS ===== */
    printf("\r\n[2] WORST-CASE EXECUTION TIME (WCET)\r\n");
    printf("--------------------------------------------------------------------\r\n");
    printf("Task               WCET (us)    WCET (cycles)    Executions\r\n");
    printf("--------------------------------------------------------------------\r\n");
    printf("Acquisition        %7lu      %10lu      %lu\r\n",
           g_Metrics.wcet_acquisition.acquisition_wcet_us,
           g_Metrics.wcet_acquisition.acquisition_wcet_cycles,
           g_Metrics.wcet_acquisition.execution_count);
    printf("Processing         %7lu      %10lu      %lu\r\n",
           g_Metrics.wcet_processing.acquisition_wcet_us,
           g_Metrics.wcet_processing.acquisition_wcet_cycles,
           g_Metrics.wcet_processing.execution_count);
    printf("Analysis           %7lu      %10lu      %lu\r\n",
           g_Metrics.wcet_analysis.acquisition_wcet_us,
           g_Metrics.wcet_analysis.acquisition_wcet_cycles,
           g_Metrics.wcet_analysis.execution_count);

    /* ===== RESOURCE UTILIZATION ===== */
    printf("\r\n[3] RESOURCE UTILIZATION\r\n");
    printf("--------------------------------------------------------------------\r\n");
    printf("CPU Utilization:\r\n");
    printf("  Current:  %.2f%%\r\n", g_Metrics.resources.cpu_utilization_percent);
    printf("  Peak:     %.2f%%\r\n", g_Metrics.resources.cpu_utilization_peak_percent);

    printf("\r\nHeap Usage:\r\n");
    printf("  Used:     %lu bytes (%.2f KB)\r\n",
           g_Metrics.resources.heap_used_bytes,
           g_Metrics.resources.heap_used_bytes / 1024.0f);
    printf("  Free:     %lu bytes (%.2f KB)\r\n",
           g_Metrics.resources.heap_free_bytes,
           g_Metrics.resources.heap_free_bytes / 1024.0f);
    printf("  Min Free: %lu bytes\r\n", g_Metrics.resources.heap_min_free_bytes);

    printf("\r\nStack High-Water Marks (Minimum Free):\r\n");
    printf("  Acquisition:  %4lu bytes ", g_Metrics.resources.acquisition_stack_hwm);
    if (g_Metrics.resources.acquisition_stack_hwm < 64) printf("*** WARNING ***");
    printf("\r\n");

    printf("  Processing:   %4lu bytes ", g_Metrics.resources.processing_stack_hwm);
    if (g_Metrics.resources.processing_stack_hwm < 64) printf("*** WARNING ***");
    printf("\r\n");

    printf("  Analysis:     %4lu bytes ", g_Metrics.resources.analysis_stack_hwm);
    if (g_Metrics.resources.analysis_stack_hwm < 64) printf("*** WARNING ***");
    printf("\r\n");

    printf("  Metrics:      %4lu bytes ", g_Metrics.resources.metrics_stack_hwm);
    if (g_Metrics.resources.metrics_stack_hwm < 64) printf("*** WARNING ***");
    printf("\r\n");

    printf("\r\nQueue Occupancy (Peak):\r\n");
    printf("  Raw Samples:       %lu / %d\r\n",
           g_Metrics.resources.raw_queue_peak_usage, RAW_SAMPLE_QUEUE_SIZE);
    printf("  Processed Samples: %lu / %d\r\n",
           g_Metrics.resources.processed_queue_peak_usage, PROCESSED_QUEUE_SIZE);
    printf("  Results:           %lu / %d\r\n",
           g_Metrics.resources.results_queue_peak_usage, RESULTS_QUEUE_SIZE);

    /* ===== DATA INTEGRITY ===== */
    printf("\r\n[4] DATA INTEGRITY\r\n");
    printf("--------------------------------------------------------------------\r\n");
    printf("  Samples Acquired:     %lu\r\n", g_Metrics.integrity.samples_acquired);
    printf("  Samples Processed:    %lu\r\n", g_Metrics.integrity.samples_processed);
    printf("  Samples Analyzed:     %lu\r\n", g_Metrics.integrity.samples_analyzed);
    printf("  Samples Lost:         %lu\r\n", g_Metrics.integrity.samples_lost_overflow);
    printf("  Sequence Gaps:        %lu\r\n", g_Metrics.integrity.sequence_gaps_detected);
    printf("  ADC Faults:           %lu\r\n", g_Metrics.integrity.adc_faults);
    printf("  Plausibility Errors:  %lu\r\n", g_Metrics.integrity.plausibility_violations);

    /* ===== SAFETY STATUS ===== */
    printf("\r\n[5] SAFETY STATUS\r\n");
    printf("--------------------------------------------------------------------\r\n");
    printf("  ADC Fault:            %s\r\n", g_Metrics.safety.adc_fault ? "FAULT" : "OK");
    printf("  Queue Overflow:       %s\r\n", g_Metrics.safety.queue_overflow ? "FAULT" : "OK");
    printf("  Stack Overflow Risk:  %s\r\n", g_Metrics.safety.stack_overflow_risk ? "WARNING" : "OK");
    printf("  Implausible Flow:     %s\r\n", g_Metrics.safety.implausible_flow ? "FAULT" : "OK");
    printf("  Negative Volume:      %s\r\n", g_Metrics.safety.negative_volume ? "FAULT" : "OK");
    printf("  Integrator Overflow:  %s\r\n", g_Metrics.safety.integrator_overflow ? "FAULT" : "OK");

    printf("\r\n========================================================================\r\n");
    printf("\r\n");
}

void Metrics_PrintResearchTable(void)
{
    printf("\r\n");
    printf("========================================================================\r\n");
    printf("  RESEARCH-GRADE METRICS SUMMARY (For Publication)\r\n");
    printf("========================================================================\r\n");

    printf("\r\nTable I: Real-Time Performance Metrics\r\n");
    printf("------------------------------------------------------------------------\r\n");
    printf("Metric                      Average      Maximum      Std Dev      Unit\r\n");
    printf("------------------------------------------------------------------------\r\n");
    printf("E2E Latency                 %7.2f      %7.2f      N/A          us\r\n",
           g_Metrics.timing.e2e_latency_avg_us,
           g_Metrics.timing.e2e_latency_max_us);
    printf("Sampling Jitter             %7.2f      %7.2f      N/A          us\r\n",
           g_Metrics.timing.jitter_avg_us,
           g_Metrics.timing.jitter_max_us);
    printf("Deadline Miss Rate          %.4f%%     N/A          N/A          %%\r\n",
           g_Metrics.timing.deadline_miss_rate_percent);

    printf("\r\nTable II: Task WCET Analysis\r\n");
    printf("------------------------------------------------------------------------\r\n");
    printf("Task                        WCET (us)    Priority     Period (ms)\r\n");
    printf("------------------------------------------------------------------------\r\n");
    printf("Acquisition                 %7lu      3            10\r\n",
           g_Metrics.wcet_acquisition.acquisition_wcet_us);
    printf("Processing                  %7lu      2            Event\r\n",
           g_Metrics.wcet_processing.acquisition_wcet_us);
    printf("Analysis                    %7lu      1            Event\r\n",
           g_Metrics.wcet_analysis.acquisition_wcet_us);

    printf("\r\nTable III: Resource Utilization\r\n");
    printf("------------------------------------------------------------------------\r\n");
    printf("Resource                    Current      Peak         Unit\r\n");
    printf("------------------------------------------------------------------------\r\n");
    printf("CPU Utilization             %.2f%%       %.2f%%       %%\r\n",
           g_Metrics.resources.cpu_utilization_percent,
           g_Metrics.resources.cpu_utilization_peak_percent);
    printf("Heap Used                   %.2f        N/A          KB\r\n",
           g_Metrics.resources.heap_used_bytes / 1024.0f);
    printf("Min Stack Free (Acq)        %lu          N/A          bytes\r\n",
           g_Metrics.resources.acquisition_stack_hwm);

    printf("\r\n========================================================================\r\n");
    printf("\r\n");
}

/* ============================================================================
   METRICS TASK
   ============================================================================ */

void Task_Metrics(void *pvParameters)
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(METRICS_REPORT_PERIOD_MS);

    xLastWakeTime = xTaskGetTickCount();

    printf("Metrics Task Started (Priority: %d)\r\n",
           (int)uxTaskPriorityGet(NULL));

    for (;;)
    {
        // Collect all metrics
        Metrics_CollectCPU();
        Metrics_CollectHeap();
        Metrics_CollectStack();
        Metrics_CollectQueues();

        // Update uptime
        g_Metrics.system_uptime_seconds = xTaskGetTickCount() / 1000;

        // Print report
        Metrics_PrintReport();

        // Optional: Print research table every 30 seconds
        static uint32_t report_counter = 0;
        report_counter++;

        if (report_counter % 6 == 0) {  // Every 30 seconds if period is 5s
            Metrics_PrintResearchTable();
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}
