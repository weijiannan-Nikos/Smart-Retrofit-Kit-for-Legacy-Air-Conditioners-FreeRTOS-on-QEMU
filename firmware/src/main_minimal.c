/**
 * @file main_minimal.c
 * @brief 最小化主程序 - 避免QEMU崩溃
 * @author jiannan WEI (MC555577)
 */

int main(void)
{
    /* 简单的无限循环，按Ctrl+C退出QEMU */
    volatile int i = 0;
    while(i < 10) {
        i++;
    }
    return 0;
}