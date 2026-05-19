/* jiannan WEI MC555577 2025/11/11 FOR EMBEDDED SYSTEM */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "ir_hardware.h"
#include "esp8266.h"
#include "sht30.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* System Configuration */
#define TEMP_QUEUE_SIZE 5
#define HUMIDITY_QUEUE_SIZE 5
#define CONTROL_QUEUE_SIZE 5
#define MAX_SCHEDULE_ENTRIES 10
#define BUF_SMALL 64
#define BUF_MEDIUM 128
#define BUF_LARGE 256

/* Event Group Bits */
#define TEMP_CHANGED_BIT    (1 << 0)
#define AC_CONTROL_BIT      (1 << 1)
#define WEATHER_UPDATE_BIT  (1 << 2)
#define SCHEDULE_ACTIVE_BIT (1 << 3)

/* IR Control Definitions */
typedef enum {
    IR_CMD_POWER_ON = 0x01,
    IR_CMD_POWER_OFF = 0x02,
    IR_CMD_MODE_AUTO = 0x10,
    IR_CMD_MODE_COOL = 0x11,
    IR_CMD_MODE_HEAT = 0x12,
    IR_CMD_MODE_DRY = 0x13,
    IR_CMD_MODE_FAN = 0x14,
    IR_CMD_TEMP_UP = 0x20,
    IR_CMD_TEMP_DOWN = 0x21,
    IR_CMD_FAN_LOW = 0x30,
    IR_CMD_FAN_MED = 0x31,
    IR_CMD_FAN_HIGH = 0x32,
    IR_CMD_FAN_AUTO = 0x33,
    IR_CMD_TIMER_ON = 0x40,
    IR_CMD_TIMER_OFF = 0x41,
    IR_CMD_SCHEDULE_POWER_ON = 0x42,
    IR_CMD_SCHEDULE_POWER_OFF = 0x43,
    IR_CMD_CLEAR_SCHEDULE = 0x44,
    IR_CMD_SWING = 0x50,
    IR_CMD_DIRECTION_AUTO = 0x60,
    IR_CMD_DIRECTION_UP = 0x61,
    IR_CMD_DIRECTION_DOWN = 0x62
} IRCommand_t;

typedef struct {
    IRCommand_t cmd;
    int param;
} IRPacket_t;

/* IR Feedback from AC */
typedef enum {
    AC_ACK_OK = 0x00,
    AC_ACK_ERROR = 0x01,
    AC_STATUS_RUNNING = 0x10,
    AC_STATUS_STANDBY = 0x11,
    AC_ERROR_SENSOR = 0x20,
    AC_ERROR_COMPRESSOR = 0x21,
    AC_ERROR_FILTER = 0x22
} ACFeedback_t;

typedef struct {
    ACFeedback_t feedback;
    int actual_temp;      // AC's internal temperature reading
    int actual_mode;      // 0=auto, 1=cool, 2=heat, 3=dry, 4=fan
    int actual_fan;       // 0=auto, 1=low, 2=med, 3=high
    int error_code;       // Error code if any
    TickType_t timestamp;
} ACResponse_t;

/* Data Structures */
typedef struct {
    int temperature;
    TickType_t timestamp;
} TempData_t;

typedef struct {
    int humidity;
    TickType_t timestamp;
} HumidityData_t;

typedef struct {
    char command[32];
    int target_temp;
    TickType_t timestamp;
} ControlCommand_t;

typedef struct {
    int hour;
    int minute;
    int target_temp;
    int enabled;
} ScheduleEntry_t;

typedef struct {
    int hour;
    int minute;
    int power_on;  // 1=开机, 0=关机
    int enabled;
} PowerScheduleEntry_t;

typedef struct {
    int forecast_temp;
    int forecast_humidity;
    char condition[32];
    int wind_speed;
    int month;  // 1-12 for season detection
} WeatherData_t;

typedef struct {
    int suggested_temp;
    char reason[64];
} TempSuggestion_t;

/* Global Variables */
static QueueHandle_t xTempQueue;
static QueueHandle_t xHumidityQueue;
static QueueHandle_t xControlQueue;
static SemaphoreHandle_t xControlMutex;
static SemaphoreHandle_t xWeatherMutex;
static EventGroupHandle_t xSystemEvents;

static ControlCommand_t lastControlCommand;
static WeatherData_t currentWeather;
static ScheduleEntry_t scheduleEntries[MAX_SCHEDULE_ENTRIES];
static int currentTemp = 25;
static int currentHumidity = 50;
static int scheduleCount = 0;
static TempSuggestion_t currentSuggestion;
static int acPowerOn = 0;
static int targetTemp = 24;
static char acMode[16] = "auto";
static int fanSpeed = 2;  // 1=Low, 2=Med, 3=High, 0=Auto
static int swingMode = 0;  // 0=Off, 1=On
static int timerHours = 0;  // 0=Off, 1-24=Timer hours
static char windDirection[8] = "auto";  // auto/up/down
static PowerScheduleEntry_t powerOnSchedule = {7, 0, 1, 0};   // 7:00开机
static PowerScheduleEntry_t powerOffSchedule = {23, 0, 0, 0}; // 23:00关机
static QueueHandle_t xACResponseQueue;  // Queue for AC feedback
static QueueHandle_t xIRTxQueue;  // Queue for IR transmission to virtual AC
static ACResponse_t lastACResponse;  // Last received AC response

/* Virtual AC State */
typedef struct {
    int power;           // 0=off, 1=on
    int set_temp;        // 16-35
    int current_temp;    // Simulated room temp
    int mode;            // 0=auto, 1=cool, 2=heat, 3=dry, 4=fan
    int fan;             // 0=auto, 1=low, 2=med, 3=high
    int swing;           // 0=off, 1=on
    int timer;           // 0-24 hours
    int error_state;     // 0=ok, >0=error code
} VirtualAC_t;

static VirtualAC_t virtualAC = {0, 24, 28, 0, 2, 0, 0, 0};

/* WiFi Time Sync */
static uint8_t systemHour = 0;
static uint8_t systemMinute = 0;
static uint8_t systemSecond = 0;
static uint8_t timeValid = 0;

/* WiFi SmartConfig */
static uint8_t wifiConfigured = 1;  // QEMU Demo: Pre-configured WiFi
static uint8_t clearWiFiOnBoot = 0;  // Set to 1 to clear WiFi on next boot

/* IR Simulation Functions - Using VS1838B + IR333C */
static void send_ir_signal(IRCommand_t cmd, int param) {
    // Use IR333C LED to transmit NEC protocol
    uint16_t address = 0x20DF;  // LG AC address code
    uint8_t command = (uint8_t)cmd;
    
    // Transmit via IR333C (940nm LED with 38kHz carrier)
    IR_TX_Send_NEC(address, command);
    
    // Send to virtual AC via queue (simulating wireless transmission)
    IRPacket_t packet = {cmd, param};
    xQueueSend(xIRTxQueue, &packet, 0);
}

static void ir_send_power(int on) {
    send_ir_signal(on ? IR_CMD_POWER_ON : IR_CMD_POWER_OFF, 0);
    printf("[IR] Power %s\n", on ? "ON" : "OFF");
}

static void ir_send_mode(const char *mode) {
    IRCommand_t cmd = IR_CMD_MODE_AUTO;
    if (strcmp(mode, "cool") == 0) cmd = IR_CMD_MODE_COOL;
    else if (strcmp(mode, "heat") == 0) cmd = IR_CMD_MODE_HEAT;
    else if (strcmp(mode, "dry") == 0) cmd = IR_CMD_MODE_DRY;
    else if (strcmp(mode, "fan") == 0) cmd = IR_CMD_MODE_FAN;
    
    send_ir_signal(cmd, 0);
    printf("[IR] Mode: %s\n", mode);
}

static void ir_send_temperature(int temp) {
    if (temp < 16) temp = 16;
    if (temp > 35) temp = 35;
    
    send_ir_signal(IR_CMD_TEMP_UP, temp);
    printf("[IR] Temperature: %d°C\n", temp);
}

static void ir_send_fan_speed(int speed) {
    IRCommand_t cmd;
    const char *speed_str;
    
    switch(speed) {
        case 1: cmd = IR_CMD_FAN_LOW; speed_str = "Low"; break;
        case 2: cmd = IR_CMD_FAN_MED; speed_str = "Medium"; break;
        case 3: cmd = IR_CMD_FAN_HIGH; speed_str = "High"; break;
        default: cmd = IR_CMD_FAN_AUTO; speed_str = "Auto"; break;
    }
    
    send_ir_signal(cmd, speed);
    printf("[IR] Fan Speed: %s\n", speed_str);
}

static void ir_send_timer(int hours) {
    if (hours < 0) hours = 0;
    if (hours > 24) hours = 24;
    
    send_ir_signal(hours > 0 ? IR_CMD_TIMER_ON : IR_CMD_TIMER_OFF, hours);
    printf("[IR] Timer: %d hours\n", hours);
}

static void ir_send_swing(int on) {
    send_ir_signal(IR_CMD_SWING, on);
    printf("[IR] Swing: %s\n", on ? "ON" : "OFF");
}

static void ir_send_direction(const char *dir) {
    IRCommand_t cmd = IR_CMD_DIRECTION_AUTO;
    if (strcmp(dir, "up") == 0) cmd = IR_CMD_DIRECTION_UP;
    else if (strcmp(dir, "down") == 0) cmd = IR_CMD_DIRECTION_DOWN;
    
    send_ir_signal(cmd, 0);
    printf("[IR] Wind Direction: %s\n", dir);
}

static void ir_send_schedule_power_on(int hour, int minute) {
    int param = (hour << 8) | minute;
    send_ir_signal(IR_CMD_SCHEDULE_POWER_ON, param);
    printf("[IR] Schedule Power ON: %02d:%02d\n", hour, minute);
}

static void ir_send_schedule_power_off(int hour, int minute) {
    int param = (hour << 8) | minute;
    send_ir_signal(IR_CMD_SCHEDULE_POWER_OFF, param);
    printf("[IR] Schedule Power OFF: %02d:%02d\n", hour, minute);
}

static void ir_send_clear_schedule(void) {
    send_ir_signal(IR_CMD_CLEAR_SCHEDULE, 0);
    printf("[IR] Clear All Schedules\n");
}

/* Utility Functions */
#define log_line(s) printf("%s\n", s)

static int16_t cached_temp = 2500;
static uint16_t cached_hum = 5000;

static void read_sensor_data(void) {
    SHT30_ReadTempHumidity(&cached_temp, &cached_hum);
}

static int simulate_temp_sensor(void) {
    return cached_temp / 100;
}

static int simulate_humidity_sensor(void) {
    return cached_hum / 100;
}

static const char* const season_conditions[] = {"Cold", "Cold", "Mild", "Mild", "Mild", "Hot", "Hot", "Hot", "Cool", "Cool", "Cool", "Cold"};

static void simulate_weather_api(WeatherData_t *weather) {
    TickType_t tick = xTaskGetTickCount();
    weather->month = ((tick / 10000) % 12) + 1;
    int idx = weather->month - 1;
    
    if (weather->month >= 6 && weather->month <= 8)
        weather->forecast_temp = 28 + (tick % 8);
    else if (weather->month >= 12 || weather->month <= 2)
        weather->forecast_temp = 5 + (tick % 10);
    else if (weather->month >= 3 && weather->month <= 5)
        weather->forecast_temp = 18 + (tick % 8);
    else
        weather->forecast_temp = 15 + (tick % 10);
    
    strncpy(weather->condition, season_conditions[idx], sizeof(weather->condition) - 1);
    weather->condition[sizeof(weather->condition) - 1] = '\0';
    weather->forecast_humidity = 45 + (tick % 30);
    weather->wind_speed = 5 + (tick % 15);
}

static void generate_temp_suggestion(const WeatherData_t *weather, TempSuggestion_t *suggestion) {
    int temp = weather->forecast_temp;
    int month = weather->month;
    
    if (month >= 6 && month <= 8) {
        suggestion->suggested_temp = (temp > 30) ? 24 : (temp > 25) ? 26 : 27;
        snprintf(suggestion->reason, sizeof(suggestion->reason), "Summer %d°C->%d°C", temp, suggestion->suggested_temp);
    } else if (month >= 12 || month <= 2) {
        suggestion->suggested_temp = (temp < 5) ? 22 : (temp < 10) ? 20 : 18;
        snprintf(suggestion->reason, sizeof(suggestion->reason), "Winter %d°C->%d°C", temp, suggestion->suggested_temp);
    } else if (month >= 3 && month <= 5) {
        suggestion->suggested_temp = (temp > 22) ? 25 : (temp < 15) ? 20 : 22;
        snprintf(suggestion->reason, sizeof(suggestion->reason), "Spring %d°C->%d°C", temp, suggestion->suggested_temp);
    } else {
        suggestion->suggested_temp = (temp > 20) ? 24 : (temp < 12) ? 20 : 21;
        snprintf(suggestion->reason, sizeof(suggestion->reason), "Autumn %d°C->%d°C", temp, suggestion->suggested_temp);
    }
    
    if (weather->forecast_humidity > 70) {
        suggestion->suggested_temp -= 1;
    }
}

/* Task 0: Virtual AC Simulator - Receives IR and sends feedback */
void VirtualACTask(void *pvParameters)
{
    (void)pvParameters;
    IRPacket_t rxPacket;
    ACResponse_t response;
    char buffer[BUF_SMALL];
    
    log_line("[Virtual AC] Started - Waiting for IR signals...");
    
    for (;;) {
        if (xQueueReceive(xIRTxQueue, &rxPacket, pdMS_TO_TICKS(100)) == pdTRUE) {
            snprintf(buffer, sizeof(buffer), "[Virtual AC] IR:0x%02X P:%d", rxPacket.cmd, rxPacket.param);
            log_line(buffer);
            
            // Process command
            response.feedback = AC_ACK_OK;
            response.error_code = 0;
            
            switch(rxPacket.cmd) {
                case IR_CMD_POWER_ON:
                    virtualAC.power = 1;
                    log_line("[Virtual AC] Power ON");
                    break;
                case IR_CMD_POWER_OFF:
                    virtualAC.power = 0;
                    log_line("[Virtual AC] Power OFF");
                    break;
                case IR_CMD_TEMP_UP:
                    virtualAC.set_temp = rxPacket.param;
                    snprintf(buffer, sizeof(buffer), "[Virtual AC] Set temp to %d°C", virtualAC.set_temp);
                    log_line(buffer);
                    break;
                case IR_CMD_MODE_AUTO:
                    virtualAC.mode = 0;
                    log_line("[Virtual AC] Mode: AUTO");
                    break;
                case IR_CMD_MODE_COOL:
                    virtualAC.mode = 1;
                    log_line("[Virtual AC] Mode: COOL");
                    break;
                case IR_CMD_MODE_HEAT:
                    virtualAC.mode = 2;
                    log_line("[Virtual AC] Mode: HEAT");
                    break;
                case IR_CMD_MODE_DRY:
                    virtualAC.mode = 3;
                    log_line("[Virtual AC] Mode: DRY");
                    break;
                case IR_CMD_MODE_FAN:
                    virtualAC.mode = 4;
                    log_line("[Virtual AC] Mode: FAN");
                    break;
                case IR_CMD_FAN_LOW:
                    virtualAC.fan = 1;
                    log_line("[Virtual AC] Fan: LOW");
                    break;
                case IR_CMD_FAN_MED:
                    virtualAC.fan = 2;
                    log_line("[Virtual AC] Fan: MEDIUM");
                    break;
                case IR_CMD_FAN_HIGH:
                    virtualAC.fan = 3;
                    log_line("[Virtual AC] Fan: HIGH");
                    break;
                case IR_CMD_FAN_AUTO:
                    virtualAC.fan = 0;
                    log_line("[Virtual AC] Fan: AUTO");
                    break;
                case IR_CMD_TIMER_ON:
                    virtualAC.timer = rxPacket.param;
                    snprintf(buffer, sizeof(buffer), "[Virtual AC] Timer: %d hours", virtualAC.timer);
                    log_line(buffer);
                    break;
                case IR_CMD_TIMER_OFF:
                    virtualAC.timer = 0;
                    log_line("[Virtual AC] Timer OFF");
                    break;
                case IR_CMD_SWING:
                    virtualAC.swing = rxPacket.param;
                    snprintf(buffer, sizeof(buffer), "[Virtual AC] Swing: %s", virtualAC.swing ? "ON" : "OFF");
                    log_line(buffer);
                    break;
                case IR_CMD_SCHEDULE_POWER_ON:
                    snprintf(buffer, sizeof(buffer), "[Virtual AC] Schedule Power ON: %02d:%02d", 
                            (rxPacket.param >> 8) & 0xFF, rxPacket.param & 0xFF);
                    log_line(buffer);
                    break;
                case IR_CMD_SCHEDULE_POWER_OFF:
                    snprintf(buffer, sizeof(buffer), "[Virtual AC] Schedule Power OFF: %02d:%02d", 
                            (rxPacket.param >> 8) & 0xFF, rxPacket.param & 0xFF);
                    log_line(buffer);
                    break;
                case IR_CMD_CLEAR_SCHEDULE:
                    log_line("[Virtual AC] Clear All Schedules");
                    break;
                default:
                    response.feedback = AC_ACK_ERROR;
                    response.error_code = 0x01;
                    log_line("[Virtual AC] Unknown command");
                    break;
            }
            
            // Simulate temperature control
            if (virtualAC.power) {
                if (virtualAC.mode == 1 && virtualAC.current_temp > virtualAC.set_temp) {
                    virtualAC.current_temp--;  // Cooling
                } else if (virtualAC.mode == 2 && virtualAC.current_temp < virtualAC.set_temp) {
                    virtualAC.current_temp++;  // Heating
                }
            }
            
            // Send feedback response
            response.actual_temp = virtualAC.current_temp;
            response.actual_mode = virtualAC.mode;
            response.actual_fan = virtualAC.fan;
            response.timestamp = xTaskGetTickCount();
            
            if ((xTaskGetTickCount() % 10) != 0) {
                xQueueSend(xACResponseQueue, &response, 0);
                printf("{\"type\":\"feedback\",\"cmd\":\"0x%02X\",\"status\":\"ok\"}\n", rxPacket.cmd);
                fflush(stdout);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Task 1: Temperature Sensor Task */
void TempSensorTask(void *pvParameters)
{
    (void)pvParameters;
    TempData_t tempData;
    ControlCommand_t initCmd;
    static int firstRun = 1;
    
    printf("[TempSensor] Started\n");
    fflush(stdout);
    
    for (;;) {
        read_sensor_data();
        tempData.temperature = simulate_temp_sensor();
        tempData.timestamp = xTaskGetTickCount();
        currentTemp = tempData.temperature;
        
        xQueueSend(xTempQueue, &tempData, 0);
        printf("[Temp] %d C\n", tempData.temperature);
        fflush(stdout);
        
        // Send initial command to start the system
        if (firstRun) {
            vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for system to initialize
            strncpy(initCmd.command, "USER_CONTROL", sizeof(initCmd.command) - 1);
            initCmd.command[sizeof(initCmd.command) - 1] = '\0';
            initCmd.target_temp = 24;
            initCmd.timestamp = xTaskGetTickCount();
            xQueueSend(xControlQueue, &initCmd, 0);
            printf("[TempSensor] Sent initial command\n");
            fflush(stdout);
            firstRun = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task 2: Humidity Sensor Task */
void HumiditySensorTask(void *pvParameters)
{
    (void)pvParameters;
    HumidityData_t humidityData;
    char buffer[BUF_SMALL];
    
    for (;;) {
        humidityData.humidity = simulate_humidity_sensor();
        humidityData.timestamp = xTaskGetTickCount();
        currentHumidity = humidityData.humidity;
        
        if (xQueueSend(xHumidityQueue, &humidityData, 0) == pdTRUE) {
            snprintf(buffer, sizeof(buffer), "[HumiditySensor] %d%%", humidityData.humidity);
            log_line(buffer);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task 3: IR Control Task */
void IRControlTask(void *pvParameters)
{
    (void)pvParameters;
    ControlCommand_t command;
    char buffer[BUF_SMALL];
    
    for (;;) {
        if (xQueueReceive(xControlQueue, &command, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xControlMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                memcpy(&lastControlCommand, &command, sizeof(ControlCommand_t));
                xSemaphoreGive(xControlMutex);
                
                snprintf(buffer, sizeof(buffer), "[IR] Processing: %s (Target: %d°C)", 
                        command.command, command.target_temp);
                log_line(buffer);
                
                // Send IR commands based on control command
                if (strcmp(command.command, "POWER_ON") == 0) {
                    ir_send_power(1);
                    acPowerOn = 1;
                } else if (strcmp(command.command, "POWER_OFF") == 0) {
                    ir_send_power(0);
                    acPowerOn = 0;
                } else if (strcmp(command.command, "SET_TEMP") == 0 || 
                           strcmp(command.command, "REMOTE_TEMP") == 0 ||
                           strcmp(command.command, "USER_CONTROL") == 0 ||
                           strcmp(command.command, "AUTO_WEATHER") == 0 ||
                           strcmp(command.command, "SCHEDULE_CONTROL") == 0) {
                    ir_send_temperature(command.target_temp);
                    targetTemp = command.target_temp;
                } else if (strcmp(command.command, "MODE_AUTO") == 0) {
                    ir_send_mode("auto");
                    strncpy(acMode, "auto", sizeof(acMode) - 1);
                } else if (strcmp(command.command, "MODE_COOL") == 0) {
                    ir_send_mode("cool");
                    strncpy(acMode, "cool", sizeof(acMode) - 1);
                } else if (strcmp(command.command, "MODE_HEAT") == 0) {
                    ir_send_mode("heat");
                    strncpy(acMode, "heat", sizeof(acMode) - 1);
                } else if (strcmp(command.command, "MODE_DRY") == 0) {
                    ir_send_mode("dry");
                    strncpy(acMode, "dry", sizeof(acMode) - 1);
                } else if (strcmp(command.command, "MODE_FAN") == 0) {
                    ir_send_mode("fan");
                    strncpy(acMode, "fan", sizeof(acMode) - 1);
                } else if (strcmp(command.command, "FAN_LOW") == 0) {
                    ir_send_fan_speed(1);
                    fanSpeed = 1;
                } else if (strcmp(command.command, "FAN_MED") == 0) {
                    ir_send_fan_speed(2);
                    fanSpeed = 2;
                } else if (strcmp(command.command, "FAN_HIGH") == 0) {
                    ir_send_fan_speed(3);
                    fanSpeed = 3;
                } else if (strcmp(command.command, "FAN_AUTO") == 0) {
                    ir_send_fan_speed(0);
                    fanSpeed = 0;
                } else if (strcmp(command.command, "TIMER") == 0) {
                    ir_send_timer(command.target_temp);
                    timerHours = command.target_temp;
                } else if (strcmp(command.command, "SWING_ON") == 0) {
                    ir_send_swing(1);
                    swingMode = 1;
                } else if (strcmp(command.command, "SWING_OFF") == 0) {
                    ir_send_swing(0);
                    swingMode = 0;
                } else if (strcmp(command.command, "DIRECTION_AUTO") == 0) {
                    ir_send_direction("auto");
                    strncpy(windDirection, "auto", sizeof(windDirection) - 1);
                } else if (strcmp(command.command, "DIRECTION_UP") == 0) {
                    ir_send_direction("up");
                    strncpy(windDirection, "up", sizeof(windDirection) - 1);
                } else if (strcmp(command.command, "DIRECTION_DOWN") == 0) {
                    ir_send_direction("down");
                    strncpy(windDirection, "down", sizeof(windDirection) - 1);
                } else {
                    snprintf(buffer, sizeof(buffer), "[IR] Unknown command: %s", command.command);
                    log_line(buffer);
                }
                
                xEventGroupSetBits(xSystemEvents, AC_CONTROL_BIT);
            }
        }
    }
}

/* Task 4: Temperature Monitor Task */
void TempMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    EventBits_t eventBits;
    int initialTemp;
    char buffer[BUF_SMALL];
    
    for (;;) {
        eventBits = xEventGroupWaitBits(xSystemEvents, AC_CONTROL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        
        if (eventBits & AC_CONTROL_BIT) {
            initialTemp = currentTemp;
            
            log_line("[TempMonitor] Starting 5-second temperature monitoring");
            
            vTaskDelay(pdMS_TO_TICKS(5000)); // 5 seconds for demo
            
            if (abs(currentTemp - initialTemp) < 2) {
                snprintf(buffer, sizeof(buffer), 
                        "[TempMonitor] WARNING: No significant temperature change detected! Retrying command...");
                log_line(buffer);
                
                if (xSemaphoreTake(xControlMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    xQueueSend(xControlQueue, &lastControlCommand, 0);
                    xSemaphoreGive(xControlMutex);
                }
            } else {
                snprintf(buffer, sizeof(buffer), 
                        "[TempMonitor] Temperature changed from %d°C to %d°C", initialTemp, currentTemp);
                log_line(buffer);
            }
        }
    }
}

/* WiFi Packet Counter (shared with ControlLogTask) */
static uint32_t g_wifiPacketCount = 0;

/* Task 5: Remote Communication Task - WiFi Data Stream */
void RemoteCommTask(void *pvParameters)
{
    (void)pvParameters;
    char jsonBuffer[BUF_LARGE];
    WiFiPacket_t rxPacket;
    ControlCmd_WiFi_t wifiCmd;
    
    for (;;) {
        if (ESP8266_ReceivePacket(&rxPacket, 10) > 0) {
            g_wifiPacketCount++;
            if (rxPacket.type == WIFI_PKT_CONTROL && ESP8266_ParseControlCmd(&rxPacket, &wifiCmd) == 0) {
                ControlCommand_t cmd;
                strncpy(cmd.command, wifiCmd.command, sizeof(cmd.command) - 1);
                cmd.command[sizeof(cmd.command) - 1] = '\0';
                cmd.target_temp = wifiCmd.value;
                cmd.timestamp = xTaskGetTickCount();
                
                if (xQueueSend(xControlQueue, &cmd, 0) == pdTRUE) {
                    printf("{\"type\":\"feedback\",\"cmd\":\"%s\",\"status\":\"queued\"}\n", wifiCmd.command);
                    fflush(stdout);
                    ESP8266_SendACK(ACK_SUCCESS, 0, "OK");
                } else {
                    printf("{\"type\":\"feedback\",\"cmd\":\"%s\",\"status\":\"failed\"}\n", wifiCmd.command);
                    fflush(stdout);
                    ESP8266_SendACK(ACK_FAILURE, 0x02, "Full");
                }
            }
        }
        
        // Send status as JSON (every 2 seconds)
        snprintf(jsonBuffer, sizeof(jsonBuffer),
                "{\"type\":\"status\",\"temp\":%d,\"humidity\":%d,\"target\":%d,"
                "\"power\":%d,\"mode\":\"%s\",\"fan\":%d,\"swing\":%d,"
                "\"direction\":\"%s\",\"forecast_temp\":%d,\"forecast_hum\":%d,"
                "\"suggestion\":%d,\"condition\":\"%s\",\"last_cmd\":\"%s\",\"cmd_ack\":%d}\n",
                currentTemp, currentHumidity, targetTemp, acPowerOn, acMode,
                fanSpeed, swingMode, windDirection,
                currentWeather.forecast_temp, currentWeather.forecast_humidity,
                currentSuggestion.suggested_temp, currentWeather.condition,
                lastControlCommand.command, 
                (lastACResponse.feedback == AC_ACK_OK) ? 1 : 0);
        printf("%s", jsonBuffer);
        fflush(stdout);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* Task 6: Weather Fetch Task - WiFi Data Stream */
void WeatherFetchTask(void *pvParameters)
{
    (void)pvParameters;
    char buffer[BUF_MEDIUM];
    WiFiPacket_t rxPacket;
    WeatherData_WiFi_t wifiWeather;
    static const char* const season_names[] = {"", "Winter", "Winter", "Spring", "Spring", "Spring",
                                  "Summer", "Summer", "Summer", "Autumn", "Autumn", "Autumn", "Winter"};
    
    for (;;) {
        // Try to receive weather data from WiFi
        if (ESP8266_ReceivePacket(&rxPacket, 100) > 0) {
            g_wifiPacketCount++;
            if (rxPacket.type == WIFI_PKT_WEATHER) {
                if (ESP8266_ParseWeatherData(&rxPacket, &wifiWeather) == 0) {
                    if (xSemaphoreTake(xWeatherMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                        currentWeather.forecast_temp = wifiWeather.temperature / 10;
                        currentWeather.forecast_humidity = wifiWeather.humidity / 10;
                        currentWeather.month = wifiWeather.month;
                        strncpy(currentWeather.condition, wifiWeather.condition, sizeof(currentWeather.condition) - 1);
                        currentWeather.condition[sizeof(currentWeather.condition) - 1] = '\0';
                        
                        generate_temp_suggestion(&currentWeather, &currentSuggestion);
                        xSemaphoreGive(xWeatherMutex);
                        
                        snprintf(buffer, sizeof(buffer), 
                                "[WiFi Weather] Season: %s, Temp: %d°C, Humidity: %d%%", 
                                season_names[currentWeather.month],
                                currentWeather.forecast_temp, currentWeather.forecast_humidity);
                        log_line(buffer);
                        
                        ESP8266_SendACK(ACK_SUCCESS, 0, "Weather data received");
                        xEventGroupSetBits(xSystemEvents, WEATHER_UPDATE_BIT);
                    }
                } else {
                    ESP8266_SendACK(ACK_FAILURE, 0x01, "Parse error");
                }
            }
        } else {
            // Fallback to simulated data
            if (xSemaphoreTake(xWeatherMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                simulate_weather_api(&currentWeather);
                generate_temp_suggestion(&currentWeather, &currentSuggestion);
                xSemaphoreGive(xWeatherMutex);
                xEventGroupSetBits(xSystemEvents, WEATHER_UPDATE_BIT);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

/* Task 7: WiFi SmartConfig & Time Sync Task */
void WiFiTimeSyncTask(void *pvParameters)
{
    (void)pvParameters;
    char buffer[BUF_MEDIUM];
    
    log_line("[WiFi] Initializing ESP8266 data stream...");
    ESP8266_Init();
    
    // QEMU Demo Mode: Pre-configured WiFi
    log_line("[WiFi] ===== QEMU Demo Mode =====");
    log_line("[WiFi] WiFi already configured (simulated)");
    log_line("[WiFi] SSID: DemoWiFi");
    log_line("[WiFi] Status: Connected");
    log_line("[WiFi] IP: 192.168.1.100");
    log_line("[WiFi] Ready for remote control");
    
    // Save demo WiFi config
    ESP8266_SaveWiFiConfig("DemoWiFi", "demo1234");
    wifiConfigured = 1;
    
    // Optional: Check if WiFi config should be cleared (for testing)
    if (clearWiFiOnBoot) {
        log_line("[WiFi] Clearing WiFi credentials (power-off reset)...");
        ESP8266_ClearWiFiConfig();
        clearWiFiOnBoot = 0;
        wifiConfigured = 0;
        
        log_line("[WiFi] Starting SmartConfig...");
        ESP8266_StartSmartConfig();
        
        // Wait for SmartConfig (max 120 seconds)
        int timeout = 120;
        while (timeout > 0) {
            SmartConfigStatus_t status = ESP8266_GetSmartConfigStatus();
            
            if (status == SC_STATUS_LINK_OVER) {
                log_line("[WiFi] SmartConfig completed!");
                char ssid[32], password[64];
                ESP8266_LoadWiFiConfig(ssid, password);
                ESP8266_SaveWiFiConfig(ssid, password);
                ESP8266_ConnectWiFi(NULL, NULL);
                wifiConfigured = 1;
                ESP8266_StopSmartConfig();
                break;
            }
            
            vTaskDelay(pdMS_TO_TICKS(1000));
            timeout--;
        }
    }
    
    // Initial time sync
    if (wifiConfigured && ESP8266_GetNTPTime(&systemHour, &systemMinute, &systemSecond)) {
        timeValid = 1;
        snprintf(buffer, sizeof(buffer), 
                "[NTP] Time synced: %02d:%02d:%02d", 
                systemHour, systemMinute, systemSecond);
        log_line(buffer);
    } else {
        log_line("[WiFi] Using system time");
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        systemHour = t->tm_hour;
        systemMinute = t->tm_min;
        systemSecond = t->tm_sec;
        timeValid = 1;
    }
    
    // Sync every hour
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(3600000));
        
        if (wifiConfigured && ESP8266_GetNTPTime(&systemHour, &systemMinute, &systemSecond)) {
            snprintf(buffer, sizeof(buffer), 
                    "[NTP] Time updated: %02d:%02d:%02d", 
                    systemHour, systemMinute, systemSecond);
            log_line(buffer);
        }
    }
}

/* Task 7.5: Time Update Task */
void TimeUpdateTask(void *pvParameters)
{
    (void)pvParameters;
    
    for (;;) {
        if (timeValid) {
            systemSecond++;
            if (systemSecond >= 60) {
                systemSecond = 0;
                systemMinute++;
                if (systemMinute >= 60) {
                    systemMinute = 0;
                    systemHour++;
                    if (systemHour >= 24) {
                        systemHour = 0;
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task 8: Schedule Task */
void ScheduleTask(void *pvParameters)
{
    (void)pvParameters;
    ControlCommand_t scheduleCommand;
    char buffer[BUF_MEDIUM];
    int lastMinute = -1;
    int conflictDetected = 0;
    
    for (;;) {
        // Wait for time to be valid
        if (!timeValid) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }
        
        // Only check once per minute
        if (systemMinute == lastMinute) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            continue;
        }
        lastMinute = systemMinute;
        
        snprintf(buffer, sizeof(buffer), 
                "[Schedule] Current time: %02d:%02d:%02d", 
                systemHour, systemMinute, systemSecond);
        log_line(buffer);
        
        // Check for schedule conflicts
        conflictDetected = 0;
        if (powerOnSchedule.enabled && powerOffSchedule.enabled &&
            powerOnSchedule.hour == powerOffSchedule.hour &&
            powerOnSchedule.minute == powerOffSchedule.minute) {
            log_line("[Schedule] CONFLICT: Power ON and OFF scheduled at same time!");
            conflictDetected = 1;
        }
        
        // Check power on schedule
        if (powerOnSchedule.enabled && 
            powerOnSchedule.hour == systemHour && 
            powerOnSchedule.minute == systemMinute && !conflictDetected) {
            
            snprintf(scheduleCommand.command, sizeof(scheduleCommand.command), "POWER_ON");
            scheduleCommand.target_temp = 0;
            scheduleCommand.timestamp = xTaskGetTickCount();
            xQueueSend(xControlQueue, &scheduleCommand, 0);
            
            snprintf(buffer, sizeof(buffer), 
                    "[Schedule] Power ON at %02d:%02d", systemHour, systemMinute);
            log_line(buffer);
            
            xEventGroupSetBits(xSystemEvents, SCHEDULE_ACTIVE_BIT);
        }
        
        // Check power off schedule
        if (powerOffSchedule.enabled && 
            powerOffSchedule.hour == systemHour && 
            powerOffSchedule.minute == systemMinute && !conflictDetected) {
            
            snprintf(scheduleCommand.command, sizeof(scheduleCommand.command), "POWER_OFF");
            scheduleCommand.target_temp = 0;
            scheduleCommand.timestamp = xTaskGetTickCount();
            xQueueSend(xControlQueue, &scheduleCommand, 0);
            
            snprintf(buffer, sizeof(buffer), 
                    "[Schedule] Power OFF at %02d:%02d", systemHour, systemMinute);
            log_line(buffer);
            
            xEventGroupSetBits(xSystemEvents, SCHEDULE_ACTIVE_BIT);
        }
        
        // Check temperature schedules
        for (int i = 0; i < scheduleCount; i++) {
            if (scheduleEntries[i].enabled && 
                scheduleEntries[i].hour == systemHour && 
                scheduleEntries[i].minute == systemMinute) {
                
                // Validate temperature range
                if (scheduleEntries[i].target_temp < 16 || scheduleEntries[i].target_temp > 35) {
                    snprintf(buffer, sizeof(buffer), 
                            "[Schedule] ERROR: Invalid temp %d°C at entry %d", 
                            scheduleEntries[i].target_temp, i);
                    log_line(buffer);
                    continue;
                }
                
                snprintf(scheduleCommand.command, sizeof(scheduleCommand.command), "SCHEDULE_CONTROL");
                scheduleCommand.target_temp = scheduleEntries[i].target_temp;
                scheduleCommand.timestamp = xTaskGetTickCount();
                
                xQueueSend(xControlQueue, &scheduleCommand, 0);
                
                snprintf(buffer, sizeof(buffer), 
                        "[Schedule] Triggered at %02d:%02d, Target: %d°C", 
                        systemHour, systemMinute, scheduleEntries[i].target_temp);
                log_line(buffer);
                
                xEventGroupSetBits(xSystemEvents, SCHEDULE_ACTIVE_BIT);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* Task 9: Control Log Task - WiFi Status Monitor */
void ControlLogTask(void *pvParameters)
{
    (void)pvParameters;
    char buffer[BUF_MEDIUM];
    static uint32_t lastPacketCount = 0;
    
    for (;;) {
        if (xSemaphoreTake(xControlMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            snprintf(buffer, sizeof(buffer), 
                    "[ControlLog] Last: %s, Target: %d°C, Time: %lu", 
                    lastControlCommand.command, lastControlCommand.target_temp, 
                    (unsigned long)lastControlCommand.timestamp);
            log_line(buffer);
            xSemaphoreGive(xControlMutex);
        }
        
        // WiFi data stream statistics
        uint32_t packetRate = g_wifiPacketCount - lastPacketCount;
        lastPacketCount = g_wifiPacketCount;
        snprintf(buffer, sizeof(buffer), 
                "[WiFi Stats] Packets: %lu, Rate: %lu/10s", 
                (unsigned long)g_wifiPacketCount, (unsigned long)packetRate);
        log_line(buffer);
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* Task 10: IR Feedback Handler Task */
void IRFeedbackTask(void *pvParameters)
{
    (void)pvParameters;
    ACResponse_t response;
    char buffer[BUF_MEDIUM];
    static const char* const mode_names[] = {"Auto", "Cool", "Heat", "Dry", "Fan"};
    static const char* const fan_names[] = {"Auto", "Low", "Medium", "High"};
    
    for (;;) {
        if (xQueueReceive(xACResponseQueue, &response, pdMS_TO_TICKS(100)) == pdTRUE) {
            lastACResponse = response;
            
            if (response.feedback == AC_ACK_OK) {
                snprintf(buffer, sizeof(buffer),
                        "[IR_RX] ACK OK - AC Temp:%d°C Mode:%s Fan:%s",
                        response.actual_temp,
                        mode_names[response.actual_mode],
                        fan_names[response.actual_fan]);
                log_line(buffer);
                
                // Verify AC is following commands
                if (abs(response.actual_temp - targetTemp) > 2) {
                    snprintf(buffer, sizeof(buffer),
                            "[IR_RX] WARNING: AC temp mismatch! Expected:%d Got:%d",
                            targetTemp, response.actual_temp);
                    log_line(buffer);
                }
            } else {
                // Handle errors
                if (response.error_code == 0xFF) {
                    log_line("[IR_RX] ERROR: No response from AC (Timeout)");
                } else if (response.error_code == AC_ERROR_SENSOR) {
                    log_line("[IR_RX] ERROR: AC sensor malfunction");
                } else if (response.error_code == AC_ERROR_COMPRESSOR) {
                    log_line("[IR_RX] ERROR: AC compressor error");
                } else if (response.error_code == AC_ERROR_FILTER) {
                    log_line("[IR_RX] ERROR: AC filter needs cleaning");
                } else {
                    snprintf(buffer, sizeof(buffer),
                            "[IR_RX] ERROR: Unknown error code 0x%02X", response.error_code);
                    log_line(buffer);
                }
            }
        }
    }
}

/* Task 11: User Interface Task */
void UserInterfaceTask(void *pvParameters)
{
    (void)pvParameters;
    ControlCommand_t userCommand;
    char buffer[BUF_MEDIUM];
    static int commandCounter = 0;
    static int firstRun = 1;
    
    // Send initial command to break deadlock
    if (firstRun) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for system initialization
        strncpy(userCommand.command, "USER_CONTROL", sizeof(userCommand.command) - 1);
        userCommand.command[sizeof(userCommand.command) - 1] = '\0';
        userCommand.target_temp = 24;
        userCommand.timestamp = xTaskGetTickCount();
        xQueueSend(xControlQueue, &userCommand, 0);
        printf("[UI] Sent initial command to start system\n");
        fflush(stdout);
        firstRun = 0;
    }
    
    for (;;) {
        commandCounter++;
        
        if (commandCounter % 30 == 0) {
            snprintf(userCommand.command, sizeof(userCommand.command), "USER_CONTROL");
            userCommand.target_temp = 22 + (commandCounter % 6);
            userCommand.timestamp = xTaskGetTickCount();
            
            if (xQueueSend(xControlQueue, &userCommand, 0) == pdTRUE) {
                snprintf(buffer, sizeof(buffer), 
                        "[UI] User set target temperature to %d°C", userCommand.target_temp);
                log_line(buffer);
            } else {
                log_line("[UI] WARNING: Control queue full, command dropped");
            }
        }
        
        if (commandCounter % 60 == 0) {
            EventBits_t weatherBits = xEventGroupWaitBits(xSystemEvents, WEATHER_UPDATE_BIT, 
                                                          pdFALSE, pdFALSE, 0);
            if (weatherBits & WEATHER_UPDATE_BIT) {
                if (xSemaphoreTake(xWeatherMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    snprintf(userCommand.command, sizeof(userCommand.command), "AUTO_WEATHER");
                    userCommand.target_temp = currentSuggestion.suggested_temp;
                    userCommand.timestamp = xTaskGetTickCount();
                    
                    if (xQueueSend(xControlQueue, &userCommand, 0) == pdTRUE) {
                        snprintf(buffer, sizeof(buffer),
                                "[UI] Auto-adjustment: Setting to %d°C based on AI suggestion",
                                currentSuggestion.suggested_temp);
                        log_line(buffer);
                    }
                    xSemaphoreGive(xWeatherMutex);
                }
            }
        }
        
        // Display comprehensive status
        snprintf(buffer, sizeof(buffer), 
                "[UI] Status | Indoor: %d°C/%d%% | Outdoor: %d°C | Target: %d°C | Power: %s | Mode: %s | Fan: %d", 
                currentTemp, currentHumidity, currentWeather.forecast_temp, targetTemp,
                acPowerOn ? "ON" : "OFF", acMode, fanSpeed);
        log_line(buffer);
        
        snprintf(buffer, sizeof(buffer),
                "[UI] AI Suggestion: %d°C | Swing: %s | Direction: %s | Timer: %dh",
                currentSuggestion.suggested_temp, swingMode ? "ON" : "OFF", 
                windDirection, timerHours);
        log_line(buffer);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* FreeRTOS Hook Functions */
void vApplicationMallocFailedHook(void)
{
    printf("[ERROR] Malloc failed!\n");
    fflush(stdout);
    for(;;);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("[ERROR] Stack overflow in task: %s\n", pcTaskName);
    fflush(stdout);
    for(;;);
}

void vApplicationIdleHook(void)
{
    // Idle hook - do nothing
}

void vApplicationTickHook(void)
{
    // Tick hook - do nothing
}

/* Initialize System */
void initializeSystem(void)
{
    
    // Initialize queues
    xTempQueue = xQueueCreate(TEMP_QUEUE_SIZE, sizeof(TempData_t));
    xHumidityQueue = xQueueCreate(HUMIDITY_QUEUE_SIZE, sizeof(HumidityData_t));
    xControlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, sizeof(ControlCommand_t));
    xACResponseQueue = xQueueCreate(10, sizeof(ACResponse_t));
    xIRTxQueue = xQueueCreate(10, sizeof(IRPacket_t));
    
    // Check queue creation
    if (!xTempQueue || !xHumidityQueue || !xControlQueue || !xACResponseQueue || !xIRTxQueue) {
        for(;;);  // Halt on error
    }
    
    // Initialize synchronization objects
    xControlMutex = xSemaphoreCreateMutex();
    xWeatherMutex = xSemaphoreCreateMutex();
    xSystemEvents = xEventGroupCreate();
    
    // Check synchronization object creation
    if (!xControlMutex || !xWeatherMutex || !xSystemEvents) {
        for(;;);  // Halt on error
    }
    
    // Initialize default schedule
    scheduleEntries[0] = (ScheduleEntry_t){8, 0, 22, 1};   // 8:00 AM - 22°C
    scheduleEntries[1] = (ScheduleEntry_t){18, 0, 24, 1};  // 6:00 PM - 24°C
    scheduleEntries[2] = (ScheduleEntry_t){22, 0, 20, 1};  // 10:00 PM - 20°C
    scheduleCount = 3;
    
    // Initialize weather data
    strncpy(currentWeather.condition, "Sunny", sizeof(currentWeather.condition) - 1);
    currentWeather.condition[sizeof(currentWeather.condition) - 1] = '\0';
    currentWeather.forecast_temp = 25;
    currentWeather.forecast_humidity = 50;
    currentWeather.wind_speed = 10;
    currentWeather.month = 7; // July
    
    // Initialize suggestion
    currentSuggestion.suggested_temp = 24;
    strncpy(currentSuggestion.reason, "Initializing...", sizeof(currentSuggestion.reason) - 1);
    currentSuggestion.reason[sizeof(currentSuggestion.reason) - 1] = '\0';
    
    // Initialize control command
    strncpy(lastControlCommand.command, "INIT", sizeof(lastControlCommand.command) - 1);
    lastControlCommand.command[sizeof(lastControlCommand.command) - 1] = '\0';
    lastControlCommand.target_temp = 25;
    lastControlCommand.timestamp = 0;
    
    // Initialize AC response
    lastACResponse.feedback = AC_STATUS_STANDBY;
    lastACResponse.actual_temp = 25;
    lastACResponse.actual_mode = 0;
    lastACResponse.actual_fan = 2;
    lastACResponse.error_code = 0;
    lastACResponse.timestamp = 0;
}

int main(void)
{
    // Initialize system first
    initializeSystem();
    
    // Initialize hardware (simplified versions)
    SHT30_Init();
    IR_Hardware_Init();
    
    // Create all 13 tasks with optimized stack sizes
    BaseType_t result;
    
    result = xTaskCreate(VirtualACTask, "VirtualAC", 384, NULL, 4, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create VirtualACTask");
        return -1;
    }
    
    result = xTaskCreate(TempSensorTask, "TempSensor", 256, NULL, 3, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create TempSensorTask");
        return -1;
    }
    
    result = xTaskCreate(HumiditySensorTask, "HumiditySensor", 256, NULL, 3, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create HumiditySensorTask");
        return -1;
    }
    
    result = xTaskCreate(IRControlTask, "IRControl", 384, NULL, 4, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create IRControlTask");
        return -1;
    }
    
    result = xTaskCreate(TempMonitorTask, "TempMonitor", 256, NULL, 2, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create TempMonitorTask");
        return -1;
    }
    
    result = xTaskCreate(RemoteCommTask, "RemoteComm", 384, NULL, 2, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create RemoteCommTask");
        return -1;
    }
    
    result = xTaskCreate(WeatherFetchTask, "WeatherFetch", 384, NULL, 1, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create WeatherFetchTask");
        return -1;
    }
    
    result = xTaskCreate(WiFiTimeSyncTask, "WiFiSync", 384, NULL, 2, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create WiFiTimeSyncTask");
        return -1;
    }
    
    result = xTaskCreate(TimeUpdateTask, "TimeUpdate", 128, NULL, 1, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create TimeUpdateTask");
        return -1;
    }
    
    result = xTaskCreate(ScheduleTask, "Schedule", 384, NULL, 2, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create ScheduleTask");
        return -1;
    }
    
    result = xTaskCreate(ControlLogTask, "ControlLog", 256, NULL, 1, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create ControlLogTask");
        return -1;
    }
    
    result = xTaskCreate(IRFeedbackTask, "IRFeedback", 384, NULL, 3, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create IRFeedbackTask");
        return -1;
    }
    
    result = xTaskCreate(UserInterfaceTask, "UserInterface", 384, NULL, 2, NULL);
    if (result != pdPASS) {
        log_line("[ERROR] Failed to create UserInterfaceTask");
        return -1;
    }
    
    // Tasks created, start scheduler
    
    vTaskStartScheduler();
    
    // Should never reach here
    log_line("[ERROR] ========================================");
    log_line("[ERROR] Scheduler returned unexpectedly!");
    log_line("[ERROR] System halted");
    log_line("[ERROR] ========================================");
    for(;;);
}