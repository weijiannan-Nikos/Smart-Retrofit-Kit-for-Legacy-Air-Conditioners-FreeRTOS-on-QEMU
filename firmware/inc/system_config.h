/**
 * @file system_config.h
 * @brief 系统配置头文件
 * @author jiannan WEI (MC555577)
 */

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <stdint.h>
#include <stdio.h>
#include "minimal_libc.h"
#include "system_hal.h"

/* FreeRTOS配置 */
#define SENSOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 4)
#define IR_TASK_PRIORITY        (tskIDLE_PRIORITY + 3)
#define CONTROL_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)
#define WIFI_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)
#define BRIDGE_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

/* 硬件配置 */
#define SYSTEM_CLOCK_FREQ       25000000    /* 25MHz */
#define I2C_CLOCK_FREQ          400000      /* 400kHz */
#define UART_BAUD_RATE          115200      /* 115200 baud */
#define PWM_FREQUENCY           38000       /* 38kHz */

/* SHT30传感器配置 */
#define SHT30_I2C_ADDR          0x44
#define SHT30_CMD_MEASURE       0x2C06
#define SENSOR_READ_INTERVAL    2000        /* 2秒 */

/* 红外配置 */
#define IR_RX_PIN               0           /* PA0 */
#define IR_TX_PIN               1           /* PA1 */
#define NEC_HEADER_PULSE        9000        /* 9ms */
#define NEC_HEADER_SPACE        4500        /* 4.5ms */

/* WiFi配置 */
#define WIFI_UART_PORT          1           /* UART1 */
#define WIFI_RESET_PIN          2           /* PA2 */
#define WIFI_TIMEOUT            5000        /* 5秒 */

/* 数据结构定义 */
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} SensorData_t;

typedef struct {
    char cmd[16];
    int value;
    uint32_t timestamp;
} Command_t;

typedef struct {
    SensorData_t sensor;
    int target_temp;
    int power_state;
    char mode[16];
    int fan_speed;
    int swing_state;
    uint32_t last_update;
} SystemStatus_t;

/* 空调控制码定义 */
#define AC_POWER_ON             0x02FD00FF
#define AC_POWER_OFF            0x02FD807F
#define AC_TEMP_UP              0x02FD40BF
#define AC_TEMP_DOWN            0x02FD20DF
#define AC_MODE_AUTO            0x02FD10EF
#define AC_MODE_COOL            0x02FD30CF
#define AC_MODE_HEAT            0x02FD50AF
#define AC_FAN_AUTO             0x02FD708F
#define AC_FAN_LOW              0x02FD906F
#define AC_FAN_MED              0x02FDB04F
#define AC_FAN_HIGH             0x02FDD02F



#endif /* SYSTEM_CONFIG_H */