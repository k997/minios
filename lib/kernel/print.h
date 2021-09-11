#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
void put_char(uint8_t c);
void put_str(char *message);
void put_int(int32_t num);
#endif // __LIB