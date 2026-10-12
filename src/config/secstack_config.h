#pragma once

#include <stdint.h>

#define ALIGNMENT_FILL 0x67        // byte used to fill alignment padding of canaries
#define CANARY_SIZE 8              // pretty self-explanatory. size in bytes, of course
#define stk_elem_t int             // type of stack elements
#define DBUG_PRINTF_LIT "%d"       // printf literal used to print stack elements
#define S_CNRY_PRINTF_LIT PRIx64   // printf literal used to print struct canaries
typedef uint64_t struct_cnry_t;    // type of struct canaries
