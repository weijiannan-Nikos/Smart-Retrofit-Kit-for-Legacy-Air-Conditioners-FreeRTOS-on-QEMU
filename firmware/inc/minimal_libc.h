/**
 * @brief 最小C库函数头文件
 */

#ifndef MINIMAL_LIBC_H
#define MINIMAL_LIBC_H

#include <stddef.h>

int atoi(const char *str);
int abs(int x);
size_t strlen(const char *str);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
char *strstr(const char *haystack, const char *needle);
char *strchr(const char *str, int c);

#endif /* MINIMAL_LIBC_H */