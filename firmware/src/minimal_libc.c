/**
 * @file minimal_libc.c
 * @brief 最小C库函数实现
 * @author jiannan WEI (MC555577)
 */

#include <stddef.h>

/**
 * @brief 简单的atoi实现
 */
int atoi(const char *str)
{
    int result = 0;
    int sign = 1;
    
    if (!str) return 0;
    
    /* 跳过空白字符 */
    while (*str == ' ' || *str == '\t') str++;
    
    /* 处理符号 */
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    /* 转换数字 */
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

/**
 * @brief 简单的abs实现
 */
int abs(int x)
{
    return (x < 0) ? -x : x;
}

/**
 * @brief 简单的strlen实现
 */
size_t strlen(const char *str)
{
    size_t len = 0;
    while (*str++) len++;
    return len;
}

/**
 * @brief 简单的strcpy实现
 */
char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

/**
 * @brief 简单的strncpy实现
 */
char *strncpy(char *dest, const char *src, size_t n)
{
    char *d = dest;
    while (n-- && (*d++ = *src++));
    while (n-- > 0) *d++ = '\0';
    return dest;
}

/**
 * @brief 简单的strcmp实现
 */
int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/**
 * @brief 简单的strstr实现
 */
char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) return (char*)haystack;
    
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (!*n) return (char*)haystack;
        haystack++;
    }
    
    return NULL;
}

/**
 * @brief 简单的strchr实现
 */
char *strchr(const char *str, int c)
{
    while (*str) {
        if (*str == c) return (char*)str;
        str++;
    }
    
    return (c == '\0') ? (char*)str : NULL;
}