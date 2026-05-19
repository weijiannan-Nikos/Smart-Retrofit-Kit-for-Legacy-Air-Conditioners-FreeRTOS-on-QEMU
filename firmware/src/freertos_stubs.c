/**
 * @file freertos_stubs.c
 * @brief FreeRTOS存根函数 - QEMU环境
 * @author jiannan WEI (MC555577)
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* 简化的FreeRTOS实现用于编译测试 */

void vTaskStartScheduler(void)
{
    printf("[FREERTOS] Scheduler started - Demo Mode\n");
    printf("[SYSTEM] Smart AC Gateway v5.0 Running...\n");
    
    /* 演示模式 - 运行5分钟 */
    volatile int count = 1500; /* 5分钟 = 300秒 * 5循环/秒 */
    int cycle = 0;
    
    while(count--) {
        cycle++;
        
        /* 每30秒显示一次状态 */
        if(cycle % 150 == 0) {
            printf("\n[DEMO] === System Status (Time: %d min) ===\n", cycle/150);
            printf("[SENSOR] Temperature: 25.2C, Humidity: 52%%\n");
            printf("[WEATHER] Outdoor: 28C, Humidity: 65%%, Condition: Sunny\n");
            printf("[AC] Target: 24C, Power: ON, Mode: Cool, Fan: Auto\n");
            printf("[WIFI] Connected to SmartHome_AP, Signal: -45dBm\n");
            printf("[IR] Last command: TEMP_DOWN, Status: OK\n");
            printf("[WEB] Interface active on http://localhost:8080\n");
            printf("========================================\n\n");
        }
        
        /* 模拟任务运行 */
        for(volatile int i = 0; i < 50000; i++);
    }
    
    printf("[FREERTOS] Demo completed - 5 minutes elapsed\n");
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, 
                      const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters,
                      UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask)
{
    printf("[FREERTOS] Task created: %s\n", pcName);
    return pdPASS;
}

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize)
{
    static int queue_counter = 1;
    printf("[FREERTOS] Queue created: %d\n", queue_counter);
    return (QueueHandle_t)queue_counter++;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    static int mutex_counter = 1;
    printf("[FREERTOS] Mutex created: %d\n", mutex_counter);
    return (SemaphoreHandle_t)mutex_counter++;
}

BaseType_t xQueueSend(QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait)
{
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait)
{
    return pdFALSE; /* 无数据 */
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)
{
    return pdTRUE;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    return pdTRUE;
}

void vTaskDelay(const TickType_t xTicksToDelay)
{
    /* 简单延时 */
    for(volatile int i = 0; i < (int)xTicksToDelay * 1000; i++);
}

void vTaskDelayUntil(TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement)
{
    vTaskDelay(xTimeIncrement);
}

TickType_t xTaskGetTickCount(void)
{
    static TickType_t tick_count = 0;
    return ++tick_count;
}

void vTaskDelete(TaskHandle_t xTaskToDelete)
{
    printf("[FREERTOS] Task deleted (timeout protection)\n");
    /* 在存根实现中，任务删除只是打印消息 */
}

void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed - insufficient heap memory\n");
    printf("[SYSTEM] Graceful shutdown initiated\n");
}

void vApplicationStackOverflowHook(void *xTask, char *pcTaskName)
{
    printf("[ERROR] Stack overflow in task: %s\n", pcTaskName ? pcTaskName : "Unknown");
    printf("[SYSTEM] Graceful shutdown initiated\n");
}

/* FreeRTOS中断处理函数 - 简化实现 */
void SVC_Handler(void)
{
    /* SVC调用处理 */
}

void PendSV_Handler(void)
{
    /* 上下文切换处理 */
}

void SysTick_Handler(void)
{
    /* 系统时钟处理 */
    static volatile uint32_t tick_count = 0;
    tick_count++;
}