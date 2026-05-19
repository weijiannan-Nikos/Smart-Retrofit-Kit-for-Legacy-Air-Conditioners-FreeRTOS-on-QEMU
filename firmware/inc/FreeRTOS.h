/**
 * @file FreeRTOS.h
 * @brief FreeRTOS主头文件 - 简化版本
 * @author jiannan WEI (MC555577)
 */

#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOSConfig.h"

/* 基本类型定义 */
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;
typedef uint32_t TickType_t;
typedef int32_t BaseType_t;
typedef uint32_t UBaseType_t;
typedef uint16_t configSTACK_DEPTH_TYPE;

/* 常量定义 */
#define pdTRUE      1
#define pdFALSE     0
#define pdPASS      1
#define pdFAIL      0
#define tskIDLE_PRIORITY 0

/* 任务函数类型 */
typedef void (*TaskFunction_t)(void *);

/* 时间转换宏 */
#define pdMS_TO_TICKS(ms) ((ms) / portTICK_PERIOD_MS)
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)

/* 中断控制宏 */
#define taskDISABLE_INTERRUPTS() __asm volatile ("cpsid i")
#define taskENABLE_INTERRUPTS()  __asm volatile ("cpsie i")

/* 断言宏 */
#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* 函数声明 */
void vTaskStartScheduler(void);
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, 
                      const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters,
                      UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask);
QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize);
SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement);
TickType_t xTaskGetTickCount(void);

#endif /* FREERTOS_H */