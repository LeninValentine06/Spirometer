/**
  ******************************************************************************
  * @file    metrics_collector.h
  * @brief   Performance metrics collection for research publication
  ******************************************************************************
  */

#ifndef __METRICS_COLLECTOR_H
#define __METRICS_COLLECTOR_H

#include "spirometry_types.h"
#include <stdint.h>

/* ============================================================================
   GLOBAL METRICS INSTANCE
   ============================================================================ */

extern SystemMetrics g_Metrics;

/* ============================================================================
   METRICS UPDATE FUNCTIONS
   ============================================================================ */

/**
  * @brief Update end-to-end latency metrics
  * @param trigger_time_us: Hardware timestamp when sample was triggered
  * @param completion_time_us: Hardware timestamp when processing completed
  */
void Metrics_UpdateLatency(uint32_t trigger_time_us, uint32_t completion_time_us);

/**
  * @brief Update jitter metrics (inter-arrival variation)
  * @param arrival_time_us: Hardware timestamp of current sample arrival
  * @param nominal_period_us: Expected period (10000 us for 10ms)
  */
void Metrics_UpdateJitter(uint32_t arrival_time_us, uint32_t nominal_period_us);

/**
  * @brief Update deadline miss metrics
  * @param release_time_us: When task instance should have started
  * @param completion_time_us: When task instance actually completed
  * @param deadline_us: Relative deadline (e.g., 10000 us)
  */
void Metrics_UpdateDeadline(uint32_t release_time_us, uint32_t completion_time_us, uint32_t deadline_us);

/**
  * @brief Update WCET for a specific task
  * @param wcet: Pointer to task's WCET metrics structure
  * @param execution_cycles: Measured execution time in CPU cycles
  */
void Metrics_UpdateWCET(WCETMetrics *wcet, uint32_t execution_cycles);

/**
  * @brief Collect CPU utilization metrics
  */
void Metrics_CollectCPU(void);

/**
  * @brief Collect heap usage metrics
  */
void Metrics_CollectHeap(void);

/**
  * @brief Collect stack high-water marks
  */
void Metrics_CollectStack(void);

/**
  * @brief Collect queue occupancy metrics
  */
void Metrics_CollectQueues(void);

/**
  * @brief Check safety constraints and update flags
  * @param sample: Processed sample to validate
  */
void Metrics_CheckSafety(ProcessedSample *sample);

/**
  * @brief Reset integrator (called on safety violation or breath end)
  */
void Metrics_ResetIntegrator(void);

/* ============================================================================
   REPORTING FUNCTIONS
   ============================================================================ */

/**
  * @brief Print comprehensive metrics report
  */
void Metrics_PrintReport(void);

/**
  * @brief Print research-grade metrics table (for paper)
  */
void Metrics_PrintResearchTable(void);

/**
  * @brief Initialize metrics system
  */
void Metrics_Init(void);

#endif /* __METRICS_COLLECTOR_H */
