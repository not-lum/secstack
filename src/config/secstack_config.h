#pragma once

#include <stdint.h>

#define ALIGNMENT_FILL 0x67
#define CANARY_SIZE 8
#define stk_elem_t int
#define DBUG_PRINTF_LIT "%d"
#define S_CNRY_PRINTF_LIT PRIx64
typedef uint64_t struct_cnry_t;