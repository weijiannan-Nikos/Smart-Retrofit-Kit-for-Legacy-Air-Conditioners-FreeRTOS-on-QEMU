/**
 * @file wifi_task.c
 * @brief WiFi任务实现
 * @author jiannan WEI (MC555577)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "system_config.h"
#include "wifi_task.h"

/**
 * @brief WiFi任务主函数
 */
void vWiFiTask(void *pvParameters)
{
    printf("[WIFI] Task started\n");
    
    /* 初始化ESP8266 */
    WiFi_Init();
    
    while (1)
    {
        /* WiFi状态监控 */
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("[WIFI] Status check\n");
    }
}

/**
 * @brief WiFi初始化
 */
void WiFi_Init(void)
{
    printf("[WIFI] Initializing ESP8266...\n");
    
    /* 复位ESP8266 */
    GPIO_Write(WIFI_RESET_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    GPIO_Write(WIFI_RESET_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    /* 发送AT命令 */
    UART_SendString(WIFI_UART_PORT, "AT\r\n");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[WIFI] ESP8266 initialized\n");
}