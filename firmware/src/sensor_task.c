/**
 * @file sensor_task.c
 * @brief SHT30温湿度传感器任务
 * @author jiannan WEI (MC555577)
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "system_config.h"
#include "sensor_task.h"

extern QueueHandle_t xSensorQueue;
extern SemaphoreHandle_t xSystemMutex;

/**
 * @brief SHT30传感器初始化
 */
static void SHT30_Init(void)
{
    printf("[SENSOR] SHT30 initializing...\n");
    
    /* 发送软复位命令 */
    uint8_t reset_cmd[] = {0x30, 0xA2};
    I2C_Write(SHT30_I2C_ADDR, reset_cmd, 2);
    
    vTaskDelay(pdMS_TO_TICKS(10));
    
    printf("[SENSOR] SHT30 initialized\n");
}

/**
 * @brief 读取SHT30传感器数据
 */
static int SHT30_ReadData(SensorData_t *data)
{
    uint8_t cmd[] = {0x2C, 0x06};  /* 高精度测量命令 */
    uint8_t buffer[6];
    
    /* 发送测量命令 */
    if (I2C_Write(SHT30_I2C_ADDR, cmd, 2) != 0) {
        return -1;
    }
    
    /* 等待测量完成 */
    vTaskDelay(pdMS_TO_TICKS(15));
    
    /* 读取数据 */
    if (I2C_Read(SHT30_I2C_ADDR, buffer, 6) != 0) {
        return -1;
    }
    
    /* 解析温度数据 */
    uint16_t temp_raw = (buffer[0] << 8) | buffer[1];
    data->temperature = -45.0f + 175.0f * temp_raw / 65535.0f;
    
    /* 解析湿度数据 */
    uint16_t hum_raw = (buffer[3] << 8) | buffer[4];
    data->humidity = 100.0f * hum_raw / 65535.0f;
    
    /* 时间戳 */
    data->timestamp = xTaskGetTickCount();
    
    return 0;
}

/**
 * @brief 传感器任务主函数
 */
void vSensorTask(void *pvParameters)
{
    SensorData_t sensor_data;
    TickType_t xLastWakeTime;
    TickType_t xStartTime = xTaskGetTickCount();
    
    printf("[SENSOR] Task started\n");
    
    /* 初始化传感器 */
    SHT30_Init();
    
    /* 初始化周期性任务 */
    xLastWakeTime = xTaskGetTickCount();
    
    while (1)
    {
        /* 15秒超时检查 */
        if ((xTaskGetTickCount() - xStartTime) > pdMS_TO_TICKS(15000)) {
            printf("[SENSOR] 15-second timeout, exiting\n");
            break;
        }
        
        /* 获取互斥锁 */
        if (xSemaphoreTake(xSystemMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* 读取传感器数据 */
            if (SHT30_ReadData(&sensor_data) == 0)
            {
                printf("[SENSOR] T=%.1f°C, H=%.1f%%\n", 
                       sensor_data.temperature, sensor_data.humidity);
                
                /* 发送数据到队列 */
                if (xQueueSend(xSensorQueue, &sensor_data, 0) != pdTRUE)
                {
                    printf("[SENSOR] Queue full, data dropped\n");
                }
            }
            else
            {
                printf("[SENSOR] Read failed\n");
            }
            
            /* 释放互斥锁 */
            xSemaphoreGive(xSystemMutex);
        }
        
        /* 周期性延时 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_READ_INTERVAL));
    }
}

/**
 * @brief I2C写函数 (硬件抽象层)
 */
int I2C_Write(uint8_t addr, uint8_t *data, uint8_t len)
{
    /* QEMU中的I2C写实现 */
    printf("[I2C] Write to 0x%02X: ", addr);
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
    
    /* 模拟I2C传输延时 */
    vTaskDelay(pdMS_TO_TICKS(1));
    
    return 0;  /* 成功 */
}

/**
 * @brief I2C读函数 (硬件抽象层)
 */
int I2C_Read(uint8_t addr, uint8_t *buffer, uint8_t len)
{
    /* QEMU中的I2C读实现 */
    printf("[I2C] Read from 0x%02X: ", addr);
    
    /* 模拟SHT30数据 (25°C, 50%RH) */
    if (addr == SHT30_I2C_ADDR && len == 6) {
        buffer[0] = 0x66; buffer[1] = 0x49;  /* 温度 */
        buffer[2] = 0x92;                    /* CRC */
        buffer[3] = 0x7F; buffer[4] = 0xFF;  /* 湿度 */
        buffer[5] = 0x7E;                    /* CRC */
    }
    
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
    
    return 0;  /* 成功 */
}