#include "string.h"
#include "debug.h"
#include "global.h"

void memset(void *_dst, uint8_t value, uint32_t size)
{
    ASSERT(_dst != NULL);
    uint8_t *dst = _dst;
    while (size-- > 0)
    {
        *(dst++) = value;
    }
}

void memcpy(void *_dst, const void *_src, uint32_t size)
{
    ASSERT(_dst != NULL && _src != NULL);
    uint8_t *dst = _dst;
    const uint8_t *src = _src;
    while (size-- > 0)
    {
        *(dst++) = *(src++);
    }
}

int8_t memcmp(const void *_a, const void *_b, uint8_t size)
{
    ASSERT(_a != NULL && _b != NULL);
    const char *a = _a;
    const char *b = _b;
    while (size-- > 0)
    {
        if ((*a) != (*b))
        {
            return (*a) > (*b) ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}

char *strcpy(char *_dst, const char *_src)
{
    ASSERT(_dst != NULL && _src != NULL);
    char *str_head_p = _dst; // 字符串起始地址
    while (*(_dst++) = *(_src++))
        ;
    return str_head_p;
}

uint32_t strlen(const char *str)
{
    ASSERT(str != NULL);
    const char *p = str;
    while (*p++)
        ;
    return (p - str - 1);
}

int8_t strcmp(const char *a, const char *b)
{
    ASSERT(a != NULL && b != NULL);
    while ((*a) != 0 && (*a) == (*b))
    {
        a++;
        b++;
    }
    // 若(*a) < (*b) ，则返回 -1
    // 若(*a) > (*b) , 则返回 1 ，表达式(*a) > (*b)的值为 1
    // 若(*a) = (*b) ，则返回 0 ，表达式(*a) > (*b)的值为 0
    return (*a) < (*b) ? -1 : (*a) > (*b);
}

char *strchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    while (*str != 0)
    {
        if (*str == ch)
            return (char *)str;
        str++;
    }
    return NULL;
}

char *strrchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    const char *last_char = NULL;
    while (*str != 0)
    {
        if (*str == ch)
            last_char = str;
        str++;
    }
    return (char *)last_char;
}
char *strcat(char *_dst, const char *_src)
{
    ASSERT(_dst != NULL && _src != NULL);
    char *str = _dst;
    while (*(str++))
        ;  //此时str指向 '\0' 后的一位
    --str; //此时str指向 '\0'
    while (*(str++) = *(_src++))
        ; // str的'\0' 被新数据替换
    return _dst;
}

uint32_t strchrs(const char *str, uint8_t ch)
{
    ASSERT(str != NULL);
    uint32_t cnt = 0;
    const char *p = str;
    while (*p)
    {
        if (*p == ch)
            cnt++;
        p++;
    }
    return cnt;
}