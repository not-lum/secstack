#pragma once

#include <stdint.h>

#define SECSTACK_DBG                // undefine if you want to disable debug

#define CNRY_ALIGNMENT_FILL 0x67    // byte used to fill alignment padding of canaries
#define CANARY_SIZE 8               // pretty self-explanatory. size in bytes, of course
#define stk_elem_t int              // type of stack elements
#define STK_ELEM_MAX INT32_MAX      // max stk elem value
#define STK_ELEM_MIN INT32_MIN      // min stk elem value
#define STK_ELEM_PRINTF_FMT "%d"    // printf fmt used to print stack elements
#define STK_ELEM_SCANF_FMT  "%d"    // scanf fmt used to read stack elements
#define S_CNRY_PRINTF_LIT PRIx64    // printf fmt used to print struct canaries
typedef uint64_t struct_cnry_t;     // type of struct canaries
