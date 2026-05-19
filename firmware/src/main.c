/**
 * @file main.c
 * @brief 智能空调控制网关主程序
 * @author jiannan WEI (MC555577)
 * @version v5.0
 * @date 2025-01-11
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "system_config.h"
#include "sensor_task.h"
#include "ir_task.h"
#include "wifi_task.h"
#include "control_task.h"
#include "bridge_task.h"
#include "bridge_comm.h"

/* 任务句柄 */
TaskHandle_t xSensorTaskHandle = NULL;
TaskHandle_t xIRTaskHandle = NULL;
TaskHandle_t xWiFiTaskHandle = NULL;
TaskHandle_t xControlTaskHandle = NULL;
TaskHandle_t xBridgeTaskHandle = NULL;

/* 队列句柄 */
QueueHandle_t xSensorQueue = NULL;
QueueHandle_t xCommandQueue = NULL;
QueueHandle_t xStatusQueue = NULL;

/* 互斥锁 */
SemaphoreHandle_t xSystemMutex = NULL;

/**
 * @brief 系统初始化
 */
static void SystemInit(void)
{
    /* 初始化系统时钟 */
    SystemClock_Config();
    
    /* 初始化GPIO */
    GPIO_Init();
    
    /* 初始化I2C */
    I2C_Init();
    
    /* 初始化UART */
    UART_Init();
    
    /* 初始化PWM */
    PWM_Init();
    
    /* 不禁用中断，让FreeRTOS处理 */
}

/**
 * @brief 创建FreeRTOS对象
 */
static void CreateRTOSObjects(void)
{
    /* 创建队列 */
    xSensorQueue = xQueueCreate(5, sizeof(SensorData_t));
    xCommandQueue = xQueueCreate(3, sizeof(Command_t));
    xStatusQueue = xQueueCreate(3, sizeof(SystemStatus_t));
    
    /* 创建互斥锁 */
    xSystemMutex = xSemaphoreCreateMutex();
}



/**
 * @brief 创建应用任务
 */
static void CreateTasks(void)
{
    /* 桥接任务 - 超大栈空间 */
    xTaskCreate(vBridgeTask, "BRIDGE", 
                configMINIMAL_STACK_SIZE * 16, NULL, 
                BRIDGE_TASK_PRIORITY, &xBridgeTaskHandle);
    
    /* 传感器任务 - 超大栈空间 */
    xTaskCreate(vSensorTask, "SNS", 
                configMINIMAL_STACK_SIZE * 16, NULL, 
                SENSOR_TASK_PRIORITY, &xSensorTaskHandle);
    
    /* 控制任务 */
    xTaskCreate(vControlTask, "CTRL", 
                configMINIMAL_STACK_SIZE * 8, NULL, 
                CONTROL_TASK_PRIORITY, &xControlTaskHandle);
    
    /* 红外任务 */
    xTaskCreate(vIRTask, "IR", 
                configMINIMAL_STACK_SIZE * 8, NULL, 
                IR_TASK_PRIORITY, &xIRTaskHandle);
}

/**
 * @brief 主函数
 */
int main(void)
{
    printf("================================================================================\n");
    printf("  智能空调控制网关 v5.0 - STM32F407 + FreeRTOS\n");
    printf("  Intelligent Air Conditioner Control Gateway\n");
    printf("================================================================================\n");
    printf("  Author: jiannan WEI (MC555577)\n");
    printf("  Course: Embedded System\n");
    printf("  Platform: QEMU MPS2-AN386\n");
    printf("  Demo Mode: 5 minutes with full features\n");
    printf("================================================================================\n\n");
    
    /* 系统初始化 */
    SystemInit();
    printf("[SYSTEM] Hardware initialization completed\n");
    
    /* 创建RTOS对象 */
    CreateRTOSObjects();
    printf("[SYSTEM] FreeRTOS objects created\n");
    
    /* 创建应用任务 */
    CreateTasks();
    printf("[SYSTEM] All tasks created successfully\n");
    
    printf("\n[SYSTEM] Starting FreeRTOS scheduler...\n");
    printf("[SYSTEM] Demo will run for 5 minutes with automatic timeout\n\n");
    
    /* 启动调度器 */
    vTaskStartScheduler();
    
    /* 如果调度器返回，说明出现错误 */
    printf("[ERROR] Scheduler returned unexpectedly!\n");
    
    /* 不应该到达这里 */
    while(1) {
        printf("[ERROR] System halted!\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    return 0;
}

