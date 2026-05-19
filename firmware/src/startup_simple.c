/**
 * @file startup_simple.c
 * @brief 简化启动代码
 */

extern int main(void);

#define STACK_TOP 0x20040000

void Reset_Handler(void);
void Default_Handler(void) { while(1); }

__attribute__((section(".vectors")))
void (*const vector_table[])(void) = {
    (void (*)(void))STACK_TOP,
    Reset_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, 0, 0, 0, 0,
    Default_Handler, Default_Handler, 0, Default_Handler, Default_Handler,
};

void Reset_Handler(void)
{
    main();
    while(1);
}