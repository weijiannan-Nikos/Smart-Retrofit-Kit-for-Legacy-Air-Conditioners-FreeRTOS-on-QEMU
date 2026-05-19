/**
 * @file ir_task.h
 * @brief 红外任务头文件
 * @author jiannan WEI (MC555577)
 */

#ifndef IR_TASK_H
#define IR_TASK_H

#include "system_config.h"

/* 函数声明 */
void vIRTask(void *pvParameters);
void IR_SendCode(uint32_t code);

#endif /* IR_TASK_H */