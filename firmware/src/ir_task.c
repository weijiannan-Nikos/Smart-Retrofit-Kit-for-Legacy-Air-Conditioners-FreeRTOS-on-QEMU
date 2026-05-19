/**
 * @file ir_task.c
 * @brief 红外任务实现
 * @author jiannan WEI (MC555577)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "system_config.h"
#include "ir_task.h"

extern QueueHandle_t xCommandQueue;

/**
 * @brief 红外任务主函数
 */
void vIRTask(void *pvParameters)
{
    Command_t command;
    TickType_t xStartTime = xTaskGetTickCount();
    
    printf("[IR] Task started\n");
    
    while (1)
    {
        /* 15秒超时检查 */
        if ((xTaskGetTickCount() - xStartTime) > pdMS_TO_TICKS(15000)) {
            printf("[IR] 15-second timeout, exiting\n");
            break;
        }
        
        /* 接收控制命令 */
        if (xQueueReceive(xCommandQueue, &command, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            printf("[IR] Processing command: %s = %d\n", command.cmd, command.value);
            
            /* 发送红外信号 */
            if (strcmp(command.cmd, "power") == 0) {
                IR_SendCode(command.value ? AC_POWER_ON : AC_POWER_OFF);
            } else if (strcmp(command.cmd, "temp") == 0) {
                /* 温度调节需要多次发送 */
                for (int i = 0; i < abs(command.value - 24); i++) {
                    IR_SendCode(command.value > 24 ? AC_TEMP_UP : AC_TEMP_DOWN);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief 发送红外码
 */
void IR_SendCode(uint32_t code)
{
    printf("[IR] Sending code: 0x%08X\n", (unsigned int)code);
    
    /* 模拟红外发送 */
    GPIO_Write(IR_TX_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    GPIO_Write(IR_TX_PIN, 0);
}