/**
 * @file deadlock_detector.h
 * @brief 死锁检测和恢复模块
 * @author jiannan WEI (MC555577)
 */

#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include <stdint.h>

/* 前向声明 */
typedef void* TaskHandle_t;
typedef void* TimerHandle_t;
typedef uint32_t TickType_t;
typedef enum { eReady, eRunning, eBlocked, eSuspended, eDeleted } eTaskState;

/* 死锁检测配置 */
#define DEADLOCK_CHECK_INTERVAL     pdMS_TO_TICKS(1000)  /* 1秒检查一次 */
#define TASK_TIMEOUT_THRESHOLD      pdMS_TO_TICKS(5000)   /* 5秒超时阈值 */
#define MAX_RECOVERY_ATTEMPTS       3                     /* 最大恢复尝试次数 */

/* 任务状态监控结构 */
typedef struct {
    void* task_handle;
    char task_name[16];
    uint32_t last_activity;
    uint32_t timeout_count;
    uint32_t recovery_count;
    int last_state;
} TaskMonitor_t;

/* 死锁检测器状态 */
typedef struct {
    void* check_timer;
    TaskMonitor_t monitors[5];
    uint8_t monitor_count;
    uint32_t total_deadlocks;
    uint32_t total_recoveries;
} DeadlockDetector_t;

/* 函数声明 */
void DeadlockDetector_Init(void);
void DeadlockDetector_RegisterTask(TaskHandle_t task, const char* name);
void DeadlockDetector_UpdateActivity(TaskHandle_t task);
void DeadlockDetector_ForceRecovery(void);

#endif /* DEADLOCK_DETECTOR_H */