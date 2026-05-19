/**
 * @file control_task.c
 * @brief 控制任务实现
 * @author jiannan WEI (MC555577)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "system_config.h"
#include "control_task.h"

extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xCommandQueue;
extern QueueHandle_t xStatusQueue;

/**
 * @brief 控制任务主函数
 */
void vControlTask(void *pvParameters)
{
    SensorData_t sensor_data;
    Command_t command;
    SystemStatus_t status = {0};
    TickType_t xStartTime = xTaskGetTickCount();
    
    printf("[CONTROL] Task started\n");
    
    /* 初始化状态 */
    status.target_temp = 24;
    status.power_state = 0;
    strcpy(status.mode, "auto");
    status.fan_speed = 2;
    
    while (1)
    {
        /* 15秒超时检查 */
        if ((xTaskGetTickCount() - xStartTime) > pdMS_TO_TICKS(15000)) {
            printf("[CONTROL] 15-second timeout, exiting\n");
            break;
        }
        
        /* 接收传感器数据 */
        if (xQueueReceive(xSensorQueue, &sensor_data, 0) == pdTRUE)
        {
            status.sensor = sensor_data;
            status.last_update = xTaskGetTickCount();
        }
        
        /* 接收控制命令 */
        if (xQueueReceive(xCommandQueue, &command, 0) == pdTRUE)
        {
            printf("[CONTROL] Command: %s = %d\n", command.cmd, command.value);
            
            /* 更新状态 */
            if (strcmp(command.cmd, "power") == 0) {
                status.power_state = command.value;
            } else if (strcmp(command.cmd, "temp") == 0) {
                status.target_temp = command.value;
            } else if (strcmp(command.cmd, "fan") == 0) {
                status.fan_speed = command.value;
            }
        }
        
        /* 发送状态更新 */
        xQueueSend(xStatusQueue, &status, 0);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}