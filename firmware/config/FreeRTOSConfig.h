/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS配置文件
 * @author jiannan WEI (MC555577)
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 基本配置 - 防栈溢出版 */
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      25000000
#define configTICK_RATE_HZ                      100
#define configMAX_PRIORITIES                    3
#define configMINIMAL_STACK_SIZE                512
#define configTOTAL_HEAP_SIZE                   (128 * 1024)
#define configMAX_TASK_NAME_LEN                 8

/* 任务管理 - 启用安全机制 */
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configQUEUE_REGISTRY_SIZE               8
#define configCHECK_FOR_STACK_OVERFLOW          2  /* 启用栈溢出检测 */
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_APPLICATION_TASK_TAG          0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_TASK_NOTIFICATIONS            1  /* 启用任务通知 */
#define configUSE_MUTEX_TIMEOUT                 1  /* 启用互斥锁超时 */

/* 内存管理 */
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         0

/* 定时器 - 增强版 */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10  /* 增大队列 */
#define configTIMER_TASK_STACK_DEPTH            (configMINIMAL_STACK_SIZE * 2)  /* 增大栈 */

/* 中断优先级 - QEMU兼容设置 */
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191
#define configPRIO_BITS                         4

/* API包含 */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xQueueGetMutexHolder            1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_eTaskGetState                   1

/* 断言宏在FreeRTOS.h中已定义，这里不重复定义 */ 

/* 错误处理钩子 */
void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook(void *xTask, char *pcTaskName);

/* 中断处理函数映射 */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */