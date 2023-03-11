#include "string.h"
#include "stdio.h"
#include "syscall.h"
#include "fs.h"

/*
    将参数 ap 按照格式 format 输出到字符串 str，
    返回替换后 str 长度， format 不变
*/

uint32_t vsprintf(char *str, const char *format, va_list ap)
{
    char *buf_ptr = str;
    const char *index_ptr = format;
    // 临时存放 itoa 结果
    long long arg_int;
    char *arg_str;
    while (*index_ptr)
    {
        // 不是 % 则直接从 format 复制到 str
        if ((*index_ptr) != '%')
        {
            *(buf_ptr++) = *(index_ptr++);
            continue;
        }
        // 获得 % 后的字符
        switch (*(++index_ptr))
        {
        case 's': /* 下一个参数为字符串 */
            arg_str = va_arg(ap, char *);
            strcpy(buf_ptr, arg_str); /* 将下一个参数复制到 buf_ptr 之后，并更新 buf_ptr 位置 */
            buf_ptr += strlen(buf_ptr);
            break;
        case 'c':
            *buf_ptr++ = va_arg(ap, char);
            break;

        case 'x': /* 16 进制整数 */
            arg_int = va_arg(ap, int);
            itoa(arg_int, buf_ptr, 16);
            buf_ptr += strlen(buf_ptr);
            break;
        case 'd': /* 10 进制整数 */
            arg_int = va_arg(ap, int);
            itoa(arg_int, buf_ptr, 10);
            buf_ptr += strlen(buf_ptr);
            break;
        }
        index_ptr++;
    }
    return strlen(str);
}
void printf(const char *format, ...)
{
    va_list args;
    va_start(args, format); // args = (void*)&format;
    char buf[STDIO_BUF_SIZE] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    write(STD_OUT, buf, strlen(buf));
}
uint32_t sprintf(char *buf, const char *format, ...)
{
    va_list args;
    uint32_t retval;
    va_start(args, format); // args = (void*)&format;
    retval = vsprintf(buf, format, args);
    va_end(args);
    return retval;
}

/* 整形转换为字符，结果存入 str，并返回 str 起始地址 */
char *itoa(int value, char *str, uint8_t base)
{
    char hash[] = "0123456789ABCDEF";
    // value_start 字符串中数值部分起始下标
    // 主要用于反转字符串时跳过负号
    uint32_t value_offset = 0, i = 0;
    // 只有 10 进制考虑正负
    if (base == 10 && value < 0)
    {
        str[i++] = '-';
        value_offset = 1; // 跳过负号
        value = -value;
    }
    uint32_t unsigned_value = (unsigned)value;
    do
    {
        str[i++] = hash[unsigned_value % base];
        unsigned_value /= base;
    } while (unsigned_value); // 商不为 0 则继续
    str[i] = '\0';

    // 以上生成的字符串是逆序，对其反转
    uint32_t str_len = i;
    char tmp;
    for (i = value_offset; i < str_len / 2; i++)
    {
        tmp = str[i];
        str[i] = str[value_offset + (str_len - i - 1)];
        str[value_offset + (str_len - i - 1)] = tmp;
    }
    return str;
}