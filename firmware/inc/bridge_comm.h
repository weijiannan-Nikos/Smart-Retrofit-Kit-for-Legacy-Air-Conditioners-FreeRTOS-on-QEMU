/**
 * @file bridge_comm.h
 * @brief 网桥通信模块头文件
 * @author jiannan WEI (MC555577)
 */

#ifndef BRIDGE_COMM_H
#define BRIDGE_COMM_H

#include "system_config.h"

/* 函数声明 */
void vBridgeCommTask(void *pvParameters);
void Bridge_SendStatus(SystemStatus_t *status);
int Bridge_ReceiveCommand(Command_t *cmd);
int Bridge_ParseCommand(const char *json_str, Command_t *cmd);

/* 外部硬件函数 */
void UART_SendString(uint8_t port, const char *str);
int UART_ReceiveChar(uint8_t port);
void GPIO_Write(uint8_t pin, uint8_t value);
uint32_t GPIO_Read(uint8_t pin);

#endif /* BRIDGE_COMM_H */