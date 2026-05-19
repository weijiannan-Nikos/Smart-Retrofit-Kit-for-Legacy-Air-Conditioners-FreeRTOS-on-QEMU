/**
 * @file main_simple.c
 * @brief 简化版主程序 - 用于QEMU测试
 * @author jiannan WEI (MC555577)
 */

#include <stdint.h>

/* 简单的延时函数 */
void delay(volatile int count)
{
    while(count--);
}

/* 简单的输出函数 */
void simple_print(const char *str)
{
    /* 通过semihosting输出 */
    volatile char *uart = (volatile char*)0x40004000;
    while(*str) {
        *uart = *str++;
        delay(1000);
    }
}

/**
 * @brief 简化的主函数
 */
int main(void)
{
    simple_print("Smart AC Gateway v5.0\n");
    simple_print("Author: jiannan WEI (MC555577)\n");
    simple_print("Platform: QEMU MPS2-AN386\n");
    simple_print("Status: Running...\n");
    
    /* 主循环 */
    int counter = 0;
    while(1)
    {
        simple_print("Heartbeat: ");
        
        /* 简单的数字输出 */
        char num[10];
        int temp = counter++;
        int i = 0;
        if (temp == 0) {
            num[i++] = '0';
        } else {
            while (temp > 0) {
                num[i++] = '0' + (temp % 10);
                temp /= 10;
            }
        }
        num[i] = '\0';
        
        /* 反转字符串 */
        for (int j = 0; j < i/2; j++) {
            char t = num[j];
            num[j] = num[i-1-j];
            num[i-1-j] = t;
        }
        
        simple_print(num);
        simple_print("\n");
        
        /* 延时 */
        delay(1000000);
    }
    
    return 0;
}