/**
 * @file bridge_comm.c
 * @brief 网桥通信模块 - 与外部服务器交互
 * @author jiannan WEI (MC555577)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "system_config.h"
#include "bridge_comm.h"

extern QueueHandle_t xSensorQueue;
extern QueueHandle_t xCommandQueue;

static SystemStatus_t system_status = {0};

/**
 * @brief 发送状态数据到网桥
 */
void Bridge_SendStatus(SystemStatus_t *status)
{
    /* 通过UART1发送JSON格式状态到ESP8266 */
    char json_buffer[256];
    
    snprintf(json_buffer, sizeof(json_buffer),
        "{\"temp\":%.1f,\"humidity\":%.1f,\"target\":%d,\"power\":%d,\"mode\":\"%s\",\"fan\":%d,\"swing\":%d}\n",
        status->sensor.temperature,
        status->sensor.humidity,
        status->target_temp,
        status->power_state,
        status->mode,
        status->fan_speed,
        status->swing_state);
    
    UART_SendString(1, json_buffer);
    
    printf("[BRIDGE] Status sent: T=%.1f°C, H=%.1f%%, Target=%d°C\n",
           status->sensor.temperature,
           status->sensor.humidity,
           status->target_temp);
}

/**
 * @brief 从网桥接收命令
 */
int Bridge_ReceiveCommand(Command_t *cmd)
{
    static char rx_buffer[128];
    static int rx_index = 0;
    int c;
    
    /* 从UART1接收数据 */
    while ((c = UART_ReceiveChar(1)) != -1) {
        if (c == '\n' || c == '\r') {
            if (rx_index > 0) {
                rx_buffer[rx_index] = '\0';
                
                /* 解析JSON命令 */
                if (Bridge_ParseCommand(rx_buffer, cmd)) {
                    rx_index = 0;
                    return 1;  /* 成功接收命令 */
                }
                rx_index = 0;
            }
        } else if (rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = c;
        }
    }
    
    return 0;  /* 无新命令 */
}

/**
 * @brief 解析JSON命令
 */
int Bridge_ParseCommand(const char *json_str, Command_t *cmd)
{
    /* 简单的JSON解析 - 查找 "cmd":"value" 模式 */
    const char *cmd_pos = strstr(json_str, "\"cmd\":");
    const char *val_pos = strstr(json_str, "\"val\":");
    
    if (!cmd_pos || !val_pos) {
        return 0;
    }
    
    /* 提取命令名 */
    cmd_pos = strchr(cmd_pos, '"');
    if (cmd_pos) {
        cmd_pos = strchr(cmd_pos + 1, '"');
        if (cmd_pos) {
            cmd_pos++;
            const char *cmd_end = strchr(cmd_pos, '"');
            if (cmd_end && (cmd_end - cmd_pos) < sizeof(cmd->cmd)) {
                strncpy(cmd->cmd, cmd_pos, cmd_end - cmd_pos);
                cmd->cmd[cmd_end - cmd_pos] = '\0';
            }
        }
    }
    
    /* 提取数值 */
    val_pos = strchr(val_pos, ':');
    if (val_pos) {
        cmd->value = atoi(val_pos + 1);
    }
    
    cmd->timestamp = xTaskGetTickCount();
    
    printf("[BRIDGE] Command parsed: %s = %d\n", cmd->cmd, cmd->value);
    return 1;
}

/**
 * @brief 网桥通信任务
 */
void vBridgeCommTask(void *pvParameters)
{
    SensorData_t sensor_data;
    Command_t command;
    TickType_t xLastStatusTime = 0;
    
    printf("[BRIDGE] Communication task started\n");
    
    /* 初始化系统状态 */
    system_status.target_temp = 24;
    system_status.power_state = 0;
    strcpy(system_status.mode, "auto");
    system_status.fan_speed = 2;
    system_status.swing_state = 0;
    
    while (1)
    {
        /* 接收传感器数据 */
        if (xQueueReceive(xSensorQueue, &sensor_data, 0) == pdTRUE)
        {
            system_status.sensor = sensor_data;
            system_status.last_update = xTaskGetTickCount();
        }
        
        /* 接收来自网桥的命令 */
        if (Bridge_ReceiveCommand(&command))
        {
            /* 处理命令 */
            if (strcmp(command.cmd, "power") == 0) {
                system_status.power_state = command.value;
            } else if (strcmp(command.cmd, "temp") == 0) {
                system_status.target_temp = command.value;
            } else if (strcmp(command.cmd, "mode") == 0) {
                /* 模式需要特殊处理 */
            } else if (strcmp(command.cmd, "fan") == 0) {
                system_status.fan_speed = command.value;
            }
            
            /* 转发到控制任务 */
            xQueueSend(xCommandQueue, &command, 0);
        }
        
        /* 定期发送状态 (每500ms) */
        if ((xTaskGetTickCount() - xLastStatusTime) > pdMS_TO_TICKS(500))
        {
            Bridge_SendStatus(&system_status);
            xLastStatusTime = xTaskGetTickCount();
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));  /* 20Hz */
    }
}

