/**
 * @file startup.c
 * @brief 启动代码 - QEMU MPS2-AN386
 * @author jiannan WEI (MC555577)
 */

extern int main(void);

/* 栈顶地址 */
#define STACK_TOP 0x20040000

/* 中断向量表 */
void Reset_Handler(void);
void Default_Handler(void);
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* 向量表 */
__attribute__((section(".vectors")))
void (*const vector_table[])(void) = {
    (void (*)(void))STACK_TOP,  /* 栈顶 */
    Reset_Handler,              /* 复位向量 */
    Default_Handler,            /* NMI */
    Default_Handler,            /* HardFault */
    Default_Handler,            /* MemManage */
    Default_Handler,            /* BusFault */
    Default_Handler,            /* UsageFault */
    0, 0, 0, 0,                /* 保留 */
    SVC_Handler,               /* SVCall */
    Default_Handler,            /* Debug Monitor */
    0,                         /* 保留 */
    PendSV_Handler,            /* PendSV */
    SysTick_Handler,           /* SysTick */
};

/**
 * @brief 复位处理函数
 */
void Reset_Handler(void)
{
    /* 初始化BSS段 */
    extern unsigned int _sbss, _ebss;
    unsigned int *bss = &_sbss;
    while (bss < &_ebss) {
        *bss++ = 0;
    }
    
    /* 初始化数据段 */
    extern unsigned int _sdata, _edata, _sidata;
    unsigned int *data = &_sdata;
    unsigned int *init = &_sidata;
    while (data < &_edata) {
        *data++ = *init++;
    }
    
    /* 不手动配置NVIC，让QEMU使用默认设置 */
    
    /* 跳转到main函数 */
    main();
    
    /* 不应该返回 */
    while(1);
}

/**
 * @brief 默认中断处理函数
 */
void Default_Handler(void)
{
    /* 简单返回，避免死锁 */
}