/**
  ******************************************************************************
  * @file    spirometry_types.h
  * @brief   Data structures for RTOS spirometry system
  * @author  Research-grade implementation
  ******************************************************************************
  */

#ifndef __SPIROMETRY_TYPES_H
#define __SPIROMETRY_TYPES_H

#include <stdint.h>

/* ============================================================================
   CONFIGURATION
   ============================================================================ */

#define SAMPLING_PERIOD_MS          10u
#define SAMPLING_FREQUENCY_HZ       100u
#define SAMPLING_PERIOD_S           0.01f

#define EXHALE_THRESHOLD_LPS        0.05f
#define FLOW_DEADBAND_SLPM          1.0f
#define FVL_MAX_POINTS              800

/* Enable/disable instrumentation */
#define INSTRUMENTATION_BUILD       1

#if INSTRUMENTATION_BUILD
    #define METRICS_REPORT_PERIOD_MS    5000
    #define ENABLE_GPIO_PROFILING       1
#else
    #define METRICS_REPORT_PERIOD_MS    10000
    #define ENABLE_GPIO_PROFILING       0
#endif

/* ============================================================================
   RAW DATA STRUCTURES
   ============================================================================ */

/**
  * @brief Raw ADC sample with hardware timestamp
  */
typedef struct {
    uint32_t adc_raw;              // Raw ADC value (0-4095)
    uint32_t hw_timestamp_us;      // Hardware timer value in microseconds
    uint32_t sequence_number;      // For loss detection
} RawSample;

/**
  * @brief Processed flow and volume data
  */
typedef struct {
    float flow_lps;                // Flow in liters per second
    float volume_l;                // Cumulative volume in liters
    uint32_t hw_timestamp_us;      // Original hardware timestamp
    uint32_t sequence_number;      // Preserved from raw sample
    uint32_t processing_start_us;  // When processing began (for latency)
} ProcessedSample;

/**
  * @brief Flow-Volume loop point
  */
typedef struct {
    float volume;
    float flow;
} FVLPoint;

/**
  * @brief Complete spirometry result for one breath
  */
typedef struct {
    float fvc_l;                   // Forced Vital Capacity
    float fev1_l;                  // Forced Expiratory Volume in 1 second
    float fev6_l;                  // FEV in 6 seconds
    float pef_lps;                 // Peak Expiratory Flow
    float fev1_fvc_ratio;          // FEV1/FVC ratio

    uint32_t breath_id;            // Breath sequence number
    uint8_t breath_complete;       // 1 when breath finished

    uint16_t fvl_count;            // Number of F-V points
    FVLPoint fvl_data[FVL_MAX_POINTS];
} SpirometryResult;

/* ============================================================================
   PROCESSING STATE
   ============================================================================ */

/**
  * @brief Trapezoidal integrator state
  */
typedef struct {
    float x_prev;
    float y_prev;
} IntegratorState;

/**
  * @brief Breath detection state machine
  */
typedef enum {
    BREATH_IDLE,
    BREATH_EXHALE,
    BREATH_COMPLETE
} BreathPhase;

/**
  * @brief Breath analyzer state
  */
typedef struct {
    BreathPhase phase;
    uint32_t breath_id;
    uint32_t phase_start_time_us;
    float volume_at_phase_start;

    float fvc_l;
    float fev1_l;
    float fev6_l;
    float pef_lps;

    uint8_t fev1_captured;
    uint8_t fev6_captured;

    FVLPoint fvl_buffer[FVL_MAX_POINTS];
    uint16_t fvl_count;

    uint32_t low_flow_start_us;    // For end detection hysteresis
} BreathAnalyzer;

/* ============================================================================
   METRICS STRUCTURES
   ============================================================================ */

/**
  * @brief Timing metrics for research publication
  */
typedef struct {
    // Latency metrics
    float e2e_latency_avg_us;
    float e2e_latency_max_us;
    float e2e_latency_min_us;
    double e2e_latency_sum_us;      // For average calculation
    uint32_t latency_sample_count;

    // Jitter metrics (inter-arrival variation)
    float jitter_avg_us;
    float jitter_max_us;
    double jitter_sum_us;
    uint32_t jitter_sample_count;
    uint32_t last_arrival_time_us;

    // Deadline metrics
    uint32_t total_samples;
    uint32_t deadline_misses;
    float deadline_miss_rate_percent;

} TimingMetrics;

/**
  * @brief WCET measurements per task
  */
typedef struct {
    uint32_t acquisition_wcet_cycles;
    uint32_t acquisition_wcet_us;

    uint32_t processing_wcet_cycles;
    uint32_t processing_wcet_us;

    uint32_t analysis_wcet_cycles;
    uint32_t analysis_wcet_us;

    uint32_t current_execution_cycles;
    float current_execution_us;
    uint32_t execution_count;

} WCETMetrics;

/**
  * @brief Resource utilization metrics
  */
typedef struct {
    // CPU utilization
    float cpu_utilization_percent;
    float cpu_utilization_peak_percent;

    // Heap metrics
    uint32_t heap_used_bytes;
    uint32_t heap_free_bytes;
    uint32_t heap_min_free_bytes;

    // Stack high-water marks (in bytes)
    uint32_t acquisition_stack_hwm;
    uint32_t processing_stack_hwm;
    uint32_t analysis_stack_hwm;
    uint32_t metrics_stack_hwm;

    // Queue metrics
    uint32_t raw_queue_peak_usage;
    uint32_t processed_queue_peak_usage;
    uint32_t results_queue_peak_usage;

} ResourceMetrics;

/**
  * @brief Data integrity metrics
  */
typedef struct {
    uint32_t samples_acquired;
    uint32_t samples_processed;
    uint32_t samples_analyzed;

    uint32_t samples_lost_overflow;
    uint32_t sequence_gaps_detected;
    uint32_t adc_faults;
    uint32_t plausibility_violations;

} IntegrityMetrics;

/**
  * @brief Safety flags
  */
typedef struct {
    uint8_t adc_fault;
    uint8_t integrator_overflow;
    uint8_t implausible_flow;
    uint8_t negative_volume;
    uint8_t queue_overflow;
    uint8_t stack_overflow_risk;
} SafetyFlags;

/**
  * @brief Complete metrics package
  */
typedef struct {
    TimingMetrics timing;
    WCETMetrics wcet_acquisition;
    WCETMetrics wcet_processing;
    WCETMetrics wcet_analysis;
    ResourceMetrics resources;
    IntegrityMetrics integrity;
    SafetyFlags safety;

    uint32_t system_uptime_seconds;

} SystemMetrics;

#endif /* __SPIROMETRY_TYPES_H */
