/**
 * @file wifi_task.h
 * @brief WiFi任务头文件
 * @author jiannan WEI (MC555577)
 */

#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include "system_config.h"

/* 函数声明 */
void vWiFiTask(void *pvParameters);
void WiFi_Init(void);

#endif /* WIFI_TASK_H */