/**
 * @file bridge_task.c
 * @brief 桥接任务 - 与外部模拟器通信
 * @author jiannan WEI (MC555577)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "system_config.h"
#include "bridge_task.h"

extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xCommandQueue;
extern QueueHandle_t xStatusQueue;

static SystemStatus_t current_status = {0};

/**
 * @brief 发送状态到外部桥接器
 */
static void SendStatusToBridge(SystemStatus_t *status)
{
    /* 发送JSON格式状态到桥接服务器 */
    printf("{\"temp\":%.1f,\"humidity\":%.1f,\"target\":%d,\"power\":%d,\"mode\":\"%s\",\"fan\":%d}\n",
           status->sensor.temperature,
           status->sensor.humidity,
           status->target_temp,
           status->power_state,
           status->mode,
           status->fan_speed);
}

/**
 * @brief 从外部桥接器接收命令
 */
static int ReceiveCommandFromBridge(Command_t *cmd)
{
    /* 模拟从桥接服务器接收命令 */
    /* 在实际实现中，这里会通过TCP socket接收JSON命令 */
    return 0;  /* 暂时无新命令 */
}

/**
 * @brief 桥接任务主函数
 */
void vBridgeTask(void *pvParameters)
{
    SensorData_t sensor_data;
    Command_t command;
    TickType_t xLastWakeTime;
    TickType_t xStartTime = xTaskGetTickCount();
    
    printf("[BRIDGE] Task started\n");
    
    /* 初始化状态 */
    current_status.target_temp = 24;
    current_status.power_state = 0;
    strcpy(current_status.mode, "auto");
    current_status.fan_speed = 2;
    current_status.swing_state = 0;
    
    xLastWakeTime = xTaskGetTickCount();
    
    while (1)
    {
        /* 15秒超时检查 */
        if ((xTaskGetTickCount() - xStartTime) > pdMS_TO_TICKS(15000)) {
            printf("[BRIDGE] 15-second timeout, exiting\n");
            break;
        }
        
        /* 接收传感器数据 */
        if (xQueueReceive(xSensorQueue, &sensor_data, 0) == pdTRUE)
        {
            current_status.sensor = sensor_data;
            current_status.last_update = xTaskGetTickCount();
        }
        
        /* 接收来自外部的命令 */
        if (ReceiveCommandFromBridge(&command))
        {
            /* 转发命令到控制任务 */
            if (xQueueSend(xCommandQueue, &command, 0) != pdTRUE)
            {
                printf("[BRIDGE] Command queue full\n");
            }
        }
        
        /* 接收系统状态更新 */
        if (xQueueReceive(xStatusQueue, &current_status, 0) == pdTRUE)
        {
            /* 状态已更新 */
        }
        
        /* 定期发送状态 */
        SendStatusToBridge(&current_status);
        
        /* 延时 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));  /* 5Hz */
    }
}