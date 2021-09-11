#include "print.h"
void main(void)
{
    put_char('K');
    put_char('E');
    put_char('R');
    put_char('N');
    put_char('E');
    put_char('L');
    put_char('!');
    put_char('\b');
    put_char('\n');
    put_str("kernel string edition");
    put_char('\n');
    put_int(0x12345678);
    put_char('\n');
    put_int(123456789);
    put_char('\n');
    put_int(0x00000001);
    while (1)
        ;
}