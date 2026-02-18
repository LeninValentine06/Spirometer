/**
  ******************************************************************************
  * @file    FreeRTOSConfig.h
  * @brief   FreeRTOS configuration for STM32F4 spirometry system
  ******************************************************************************
  */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *-----------------------------------------------------------*/

/* Ensure definitions are only used by the compiler, not assembler */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
#endif

#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     (7)
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)15360)
#define configMAX_TASK_NAME_LEN                  (16)
#define configUSE_TRACE_FACILITY                 1
#define configUSE_16_BIT_TICKS                   0
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_RECURSIVE_MUTEXES              0
#define configUSE_COUNTING_SEMAPHORES            0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          (2)

/* Software timer definitions. */
#define configUSE_TIMERS                         0
#define configTIMER_TASK_PRIORITY                (2)
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      0
#define INCLUDE_vTaskCleanUpResources            0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_uxTaskGetStackHighWaterMark      1

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS       __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS       4
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Interrupt priorities used by the kernel port layer itself. */
#define configKERNEL_INTERRUPT_PRIORITY \
  (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
  (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

/* IMPORTANT: SysTick_Handler is NOT defined here because we use TIM2 for timebase */

/* ============================================================================
   RUNTIME STATS CONFIGURATION (for CPU utilization)
   ============================================================================ */

#define configGENERATE_RUN_TIME_STATS           1

extern void ConfigureTimerForRuntimeStats(void);
extern uint32_t GetRuntimeCounterValue(void);

#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() ConfigureTimerForRuntimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE()         GetRuntimeCounterValue()

/* ============================================================================
   STACK OVERFLOW DETECTION
   ============================================================================ */

#define configCHECK_FOR_STACK_OVERFLOW          2

#endif /* FREERTOS_CONFIG_H */
