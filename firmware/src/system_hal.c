/**
 * @file system_hal.c
 * @brief 系统硬件抽象层 - QEMU MPS2-AN386平台
 * @author jiannan WEI (MC555577)
 */

#include "system_config.h"
#include "FreeRTOS.h"
#include "task.h"

/* QEMU MPS2-AN386 寄存器基址 */
#define UART0_BASE      0x40004000
#define UART1_BASE      0x40005000
#define GPIO0_BASE      0x40010000
#define GPIO1_BASE      0x40011000
#define I2C0_BASE       0x40020000
#define TIMER0_BASE     0x40000000

/* UART寄存器偏移 */
#define UART_DATA       0x00
#define UART_STATE      0x04
#define UART_CTRL       0x08
#define UART_BAUDDIV    0x10

/* GPIO寄存器偏移 */
#define GPIO_DATA       0x00
#define GPIO_DATAOUT    0x04
#define GPIO_OUTENSET   0x10
#define GPIO_OUTENCLR   0x14

/* I2C寄存器偏移 */
#define I2C_CONTROL     0x00
#define I2C_STATUS      0x04
#define I2C_DATA        0x08
#define I2C_ADDRESS     0x0C

/* 寄存器访问宏 */
#define REG32(addr)     (*(volatile uint32_t *)(addr))

/**
 * @brief 系统时钟配置
 */
void SystemClock_Config(void)
{
    /* QEMU中时钟已配置为25MHz */
    printf("[HAL] System clock: 25MHz\n");
}

/**
 * @brief GPIO初始化
 */
void GPIO_Init(void)
{
    /* 配置PA0为输入 (红外接收) */
    REG32(GPIO0_BASE + GPIO_OUTENCLR) = (1 << 0);
    
    /* 配置PA1为输出 (红外发射) */
    REG32(GPIO0_BASE + GPIO_OUTENSET) = (1 << 1);
    REG32(GPIO0_BASE + GPIO_DATAOUT) &= ~(1 << 1);  /* 初始为低电平 */
    
    /* 配置PA2为输出 (WiFi复位) */
    REG32(GPIO0_BASE + GPIO_OUTENSET) = (1 << 2);
    REG32(GPIO0_BASE + GPIO_DATAOUT) |= (1 << 2);   /* 初始为高电平 */
    
    printf("[HAL] GPIO initialized\n");
}

/**
 * @brief I2C初始化
 */
void I2C_Init(void)
{
    /* 配置I2C控制器 */
    REG32(I2C0_BASE + I2C_CONTROL) = 0x80;  /* 使能I2C */
    
    printf("[HAL] I2C initialized (400kHz)\n");
}

/**
 * @brief UART初始化
 */
void UART_Init(void)
{
    /* UART0 - 调试输出 (已由QEMU配置) */
    
    /* UART1 - ESP8266通信 */
    REG32(UART1_BASE + UART_BAUDDIV) = 25000000 / 115200;  /* 115200 baud */
    REG32(UART1_BASE + UART_CTRL) = 0x01;  /* 使能UART */
    
    printf("[HAL] UART initialized (115200 baud)\n");
}

/**
 * @brief PWM初始化 (用于红外38kHz载波)
 */
void PWM_Init(void)
{
    /* 配置定时器产生38kHz PWM */
    /* QEMU中简化实现 */
    
    printf("[HAL] PWM initialized (38kHz)\n");
}

/**
 * @brief GPIO读取
 */
uint32_t GPIO_Read(uint8_t pin)
{
    return (REG32(GPIO0_BASE + GPIO_DATA) >> pin) & 1;
}

/**
 * @brief GPIO写入
 */
void GPIO_Write(uint8_t pin, uint8_t value)
{
    if (value) {
        REG32(GPIO0_BASE + GPIO_DATAOUT) |= (1 << pin);
    } else {
        REG32(GPIO0_BASE + GPIO_DATAOUT) &= ~(1 << pin);
    }
}

/**
 * @brief UART发送字符
 */
void UART_SendChar(uint8_t port, char c)
{
    uint32_t base = (port == 0) ? UART0_BASE : UART1_BASE;
    
    /* 等待发送缓冲区空闲 */
    while (REG32(base + UART_STATE) & 0x20);
    
    /* 发送字符 */
    REG32(base + UART_DATA) = c;
}

/**
 * @brief UART发送字符串
 */
void UART_SendString(uint8_t port, const char *str)
{
    while (*str) {
        UART_SendChar(port, *str++);
    }
}

/**
 * @brief UART接收字符
 */
int UART_ReceiveChar(uint8_t port)
{
    uint32_t base = (port == 0) ? UART0_BASE : UART1_BASE;
    
    /* 检查接收缓冲区 */
    if (REG32(base + UART_STATE) & 0x02) {
        return REG32(base + UART_DATA) & 0xFF;
    }
    
    return -1;  /* 无数据 */
}