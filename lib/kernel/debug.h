#ifndef __LIB_KERNEL_DEBUG_H
#define __LIB_KERNEL_DEBUG_H

void panic_spin(char* filename, int line , const char* func,const char* condition);


// "..."表示定义的宏其参数可变
// __VA_ARGS__ 代表所有与省略号相对应的参数。
#define PANIC(...) panic_spin (__FILE__,__LINE__,__func__,__VA_ARGS__)

#ifdef NDEBUG
    #define ASSERT(CONDITION) ((void)0)
#else 
                                //注意 \ 的位置
    #define ASSERT(CONDITION)   /*表达式为真执行空指令，为假则执行panic */           \
    if(CONDITION) {}else{       /*此处else必须紧跟在if(CONDITION) {}后，不能换行*/   \
    PANIC(#CONDITION); }        //符号#让编译器将宏的参数转化为字符串字面量
#endif /*__NDEBUG */

#endif  /*__KERNEL_DEBUG_H*/