/**
 * @file deadlock_detector.c
 * @brief 死锁检测和恢复模块实现
 * @author jiannan WEI (MC555577)
 */

#include "deadlock_detector.h"
#include "system_config.h"

static DeadlockDetector_t detector = {0};

/**
 * @brief 死锁检测定时器回调
 */
static void vDeadlockCheckCallback(TimerHandle_t xTimer)
{
    TickType_t current_time = xTaskGetTickCount();
    
    printf("[DEADLOCK] Checking %d tasks...\n", detector.monitor_count);
    
    for (int i = 0; i < detector.monitor_count; i++) {
        TaskMonitor_t* monitor = &detector.monitors[i];
        
        if (monitor->task_handle == NULL) continue;
        
        /* 检查任务是否还存在 */
        eTaskState current_state = eTaskGetState(monitor->task_handle);
        if (current_state == eDeleted) {
            printf("[DEADLOCK] Task %s deleted, removing from monitor\n", monitor->task_name);
            monitor->task_handle = NULL;
            continue;
        }
        
        /* 检查任务活动时间 */
        TickType_t inactive_time = current_time - monitor->last_activity;
        
        if (inactive_time > TASK_TIMEOUT_THRESHOLD) {
            monitor->timeout_count++;
            detector.total_deadlocks++;
            
            printf("[DEADLOCK] Task %s timeout! Inactive for %lu ms (count: %lu)\n", 
                   monitor->task_name, 
                   inactive_time * portTICK_PERIOD_MS,
                   monitor->timeout_count);
            
            /* 尝试恢复 */
            if (monitor->recovery_count < MAX_RECOVERY_ATTEMPTS) {
                printf("[DEADLOCK] Attempting recovery for %s (attempt %lu)\n", 
                       monitor->task_name, monitor->recovery_count + 1);
                
                /* 发送任务通知唤醒 */
                if (xTaskNotify(monitor->task_handle, 0x01, eSetBits) == pdPASS) {
                    printf("[DEADLOCK] Sent notification to %s\n", monitor->task_name);
                }
                
                monitor->recovery_count++;
                detector.total_recoveries++;
                monitor->last_activity = current_time;  /* 重置活动时间 */
            } else {
                printf("[DEADLOCK] Max recovery attempts reached for %s\n", monitor->task_name);
            }
        } else {
            /* 任务正常，重置计数器 */
            if (monitor->timeout_count > 0) {
                printf("[DEADLOCK] Task %s recovered\n", monitor->task_name);
                monitor->timeout_count = 0;
                monitor->recovery_count = 0;
            }
        }
        
        monitor->last_state = current_state;
    }
    
    /* 每30秒输出统计信息 */
    static uint32_t stats_counter = 0;
    if (++stats_counter >= 30) {
        printf("[DEADLOCK] Stats - Total deadlocks: %lu, recoveries: %lu\n", 
               detector.total_deadlocks, detector.total_recoveries);
        stats_counter = 0;
    }
}

/**
 * @brief 初始化死锁检测器
 */
void DeadlockDetector_Init(void)
{
    printf("[DEADLOCK] Initializing deadlock detector...\n");
    
    /* 清零结构体 */
    memset(&detector, 0, sizeof(detector));
    
    /* 创建检测定时器 */
    detector.check_timer = xTimerCreate(
        "DeadlockCheck",
        DEADLOCK_CHECK_INTERVAL,
        pdTRUE,  /* 自动重载 */
        NULL,
        vDeadlockCheckCallback
    );
    
    if (detector.check_timer != NULL) {
        if (xTimerStart(detector.check_timer, 0) == pdPASS) {
            printf("[DEADLOCK] Detector started successfully\n");
        } else {
            printf("[DEADLOCK] Failed to start detector timer\n");
        }
    } else {
        printf("[DEADLOCK] Failed to create detector timer\n");
    }
}

/**
 * @brief 注册任务到死锁监控
 */
void DeadlockDetector_RegisterTask(TaskHandle_t task, const char* name)
{
    if (detector.monitor_count >= 5) {
        printf("[DEADLOCK] Monitor list full, cannot register %s\n", name);
        return;
    }
    
    TaskMonitor_t* monitor = &detector.monitors[detector.monitor_count];
    monitor->task_handle = task;
    strncpy(monitor->task_name, name, sizeof(monitor->task_name) - 1);
    monitor->task_name[sizeof(monitor->task_name) - 1] = '\0';
    monitor->last_activity = xTaskGetTickCount();
    monitor->timeout_count = 0;
    monitor->recovery_count = 0;
    monitor->last_state = eReady;
    
    detector.monitor_count++;
    
    printf("[DEADLOCK] Registered task %s for monitoring\n", name);
}

/**
 * @brief 更新任务活动时间
 */
void DeadlockDetector_UpdateActivity(TaskHandle_t task)
{
    for (int i = 0; i < detector.monitor_count; i++) {
        if (detector.monitors[i].task_handle == task) {
            detector.monitors[i].last_activity = xTaskGetTickCount();
            break;
        }
    }
}

/**
 * @brief 强制恢复所有任务
 */
void DeadlockDetector_ForceRecovery(void)
{
    printf("[DEADLOCK] Force recovery initiated\n");
    
    for (int i = 0; i < detector.monitor_count; i++) {
        TaskMonitor_t* monitor = &detector.monitors[i];
        if (monitor->task_handle != NULL) {
            /* 发送紧急通知 */
            xTaskNotify(monitor->task_handle, 0xFF, eSetBits);
            monitor->last_activity = xTaskGetTickCount();
            monitor->timeout_count = 0;
            monitor->recovery_count = 0;
        }
    }
    
    printf("[DEADLOCK] Force recovery completed\n");
}