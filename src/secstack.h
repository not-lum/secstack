#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Change secstack configuration in this file:
#include "config/secstack_config.h"

#define SECSTACK_DBG

#ifdef SECSTACK_DBG
#   define ON_DEBUG(...) __VA_ARGS__
#else
#   define ON_DEBUG(...)
#endif

#define VAR_NAME(x) #x

#define STACK_INIT_DBG(stk, capacity) \
    stack_init((stk), (capacity) \
               ON_DEBUG(, \
               VAR_NAME((stk)), \
               __func__, \
               __FILE__, \
               __LINE__));


#define R_DATA (stk_elem_t *)(stk->data + left_cnry_size())
 
typedef enum {
    STACK_OK = 0,
    STACK_INIT_ALLOC_ERROR,
    STACK_CANNOT_POP_FROM_EMPTY,
    STACK_SIZE_BIGGER_THAN_CAPACITY,
    STACK_CAPACITY_BIGGER_THAN_MUSABLE,
    STACK_DATA_NULL_PTR,
    STACK_RIGHT_CANARY_SMASH_DETECTED,
    STACK_LEFT_CANARY_SMASH_DETECTED,
    STACK_REALLOC_FAIL,
    STACK_NULL,
    STACK_DATA_BAD_ALIGNMENT,
    STACK_STRUCT_LEFT_CANARY_SMASH_DETECTED,
    STACK_STRUCT_RIGHT_CANARY_SMASH_DETECTED
} StackStatus;

typedef struct stack_t {
    ON_DEBUG(
    struct_cnry_t l_canary;
    )
    uint8_t *data;
    size_t capacity;
    size_t size;
    ON_DEBUG(
    const char *_dbug_var_name;
    const char *_dbug_func_name;
    const char *_dbug_filename;
    int _dbug_line;

    struct_cnry_t r_canary;
    )
} stack_t;

StackStatus stack_init(stack_t *stk, size_t capacity
    ON_DEBUG(,
    const char *_dbug_var_name,
    const char *_dbug_func_name,
    const char *_dbug_filename,
    const int _dbug_line));
void stack_push(stack_t *stk, stk_elem_t elem, StackStatus *err);
void dump_stack(stack_t *stk, StackStatus status);
void dump_stack_to(stack_t *stk, StackStatus status, FILE *out, bool colors);
stk_elem_t stack_pop(stack_t *stk, StackStatus *err);
const char *stack_error_str(StackStatus err);
StackStatus stack_verify(stack_t *stk);
StackStatus canary_verify(stack_t *stk);
void stack_destroy(stack_t *stk);
