/**
 * @file sensor_task.h
 * @brief 传感器任务头文件
 * @author jiannan WEI (MC555577)
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "system_config.h"

/* 函数声明 */
void vSensorTask(void *pvParameters);
int I2C_Write(uint8_t addr, uint8_t *data, uint8_t len);
int I2C_Read(uint8_t addr, uint8_t *buffer, uint8_t len);

#endif /* SENSOR_TASK_H */