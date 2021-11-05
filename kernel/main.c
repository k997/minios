#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"
#include "string.h"
#include "global.h"
void main(void)
{
  char *a = "I am a";
  char *b = "I am b!";
  char *c = "";
  uint32_t a_length,b_length,c_length;
  put_str("a:");
  put_str(a);
  put_str("\nb:");
  put_str(b);
  put_str("\nc:");
  put_str(c);
  put_str("\n==================\n");

  put_str("copy a to c with strcpy:");
  put_str(strcpy(c, a));
  put_str("\n");

  b_length = strlen(b);
  put_str("copy b to c with memcpy:");
  memcpy(c, b, b_length + 1);
  put_str(c);
  put_str("\n");

  put_str("cat a to c after copy b to c :");
  put_str(strcat(c, a));
  put_str("\n");

  c_length = strlen(c);
  b_length = strlen(b);
  memset(c, 'a', c_length);
  put_str("set c to 'aaaaaa...' with memset:");
  put_str(c);
  put_str("\n");

  put_str("compare c and b  with strcmp:");
  put_int(strcmp(c, b));
  put_str("\n");

  a = "I am aaa";
  b = "I am bbb";
  a_length = strlen(a);
  b_length = strlen(b);
  put_str("compare a and b  with memcmp:");
  put_int(memcmp(a, b, a_length < b_length ? a_length : b_length));
  put_str("\n");

  put_str("b has ");
  put_int(strchrs(b, ' '));
  put_str(" space\n");
  put_str("address of left space in b:");
  put_int((int32_t)strchr(b, ' '));
  put_str("\n");

  put_str("address of right space in b:");
  put_int((int32_t)strrchr(b, ' '));
  put_str("\n");

  while (1)
    ;
}