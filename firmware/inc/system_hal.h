/**
 * @file system_hal.h
 * @brief 系统硬件抽象层头文件
 * @author jiannan WEI (MC555577)
 */

#ifndef SYSTEM_HAL_H
#define SYSTEM_HAL_H

#include <stdint.h>

/* 函数声明 */
void SystemClock_Config(void);
void GPIO_Init(void);
void I2C_Init(void);
void UART_Init(void);
void PWM_Init(void);

void GPIO_Write(uint8_t pin, uint8_t value);
uint32_t GPIO_Read(uint8_t pin);
void UART_SendChar(uint8_t port, char c);
void UART_SendString(uint8_t port, const char *str);
int UART_ReceiveChar(uint8_t port);

#endif /* SYSTEM_HAL_H */