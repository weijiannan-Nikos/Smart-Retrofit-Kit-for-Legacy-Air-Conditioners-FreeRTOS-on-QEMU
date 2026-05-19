/**
 * @file syscalls.c
 * @brief 系统调用实现 - QEMU环境
 * @author jiannan WEI (MC555577)
 */

#include <sys/stat.h>
#include <errno.h>

/* 堆管理 */
static char heap[1024];
static int heap_pos = 0;

int _sbrk(int incr)
{
    int prev_pos = heap_pos;
    heap_pos += incr;
    if (heap_pos >= sizeof(heap)) heap_pos = sizeof(heap) - 1;
    return (int)&heap[prev_pos];
}

int _write(int file, char *ptr, int len)
{
    /* QEMU semihosting输出 */
    for (int i = 0; i < len; i++) {
        /* 简单输出到调试器 */
        *(volatile char*)0x20000000 = ptr[i];
    }
    return len;
}

int _read(int file, char *ptr, int len)
{
    return 0;
}

int _close(int file)
{
    return -1;
}

int _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    return 0;
}

void _exit(int status)
{
    while(1);
}

int _kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}