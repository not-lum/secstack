#include "secstack.h"
#include "canary_utils.h"
#include "dump_funcs.h"

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdalign.h>


#define ASSERT_OK(stk) do { \
    StackStatus status = stack_verify((stk)); \
    \
    ON_DEBUG( \
    if (status != STACK_OK) { \
        dump_stack((stk), status); \
    } \
    \
    ) \
    assert(status == STACK_OK); \
} while (0)


const char *stack_error_str(StackStatus err) {
    switch (err) {
        case STACK_OK:
            return "stack ok";
        case STACK_INIT_ALLOC_ERROR:
            return "stack alloc error while initialization";
        case STACK_CANNOT_POP_FROM_EMPTY:
            return "cannot pop from empty stack";
        case STACK_SIZE_BIGGER_THAN_CAPACITY:
            return "stack size is bigger than its capacity";
        case STACK_CAPACITY_BIGGER_THAN_MUSABLE:
            return "stack capacity is bigger than size of memory allocated for it";
        case STACK_DATA_NULL_PTR:
            return "pointer to stack data is NULL";
        case STACK_LEFT_CANARY_SMASH_DETECTED:
            return "left canary smash detected";
        case STACK_RIGHT_CANARY_SMASH_DETECTED:
            return "right canary smash detected";
        case STACK_REALLOC_FAIL:
            return "stack realloc failed";
        case STACK_NULL:
            return "stack null pointer";
        case STACK_DATA_BAD_ALIGNMENT:
            return "stack data pointer is misaligned";
        case STACK_STRUCT_LEFT_CANARY_SMASH_DETECTED:
            return "struct left canary smash detected";
        case STACK_STRUCT_RIGHT_CANARY_SMASH_DETECTED:
            return "struct right canary smash detected";
        default:
            return "unknown error";
    }
}

StackStatus stack_verify(stack_t *stk) {
    if (stk == NULL) {
        return STACK_NULL;
    }

    if (!check_struct_l_canary(stk))
        return STACK_STRUCT_LEFT_CANARY_SMASH_DETECTED;

    if (!check_struct_r_canary(stk))
        return STACK_STRUCT_RIGHT_CANARY_SMASH_DETECTED;

    if (stk->size > stk->capacity) {
        return STACK_SIZE_BIGGER_THAN_CAPACITY;
    }

    if (stk->data == NULL) {
        return STACK_DATA_NULL_PTR;
    }

    if ((size_t)(stk->data) % alignof(stk_elem_t) != 0) {
        return STACK_DATA_BAD_ALIGNMENT;
    }

    if (stk->capacity * sizeof(stk_elem_t) + CANARY_SIZE + left_cnry_size() > malloc_usable_size(stk->data)) {
        return STACK_CAPACITY_BIGGER_THAN_MUSABLE;
    }

    if (!check_l_canary(stk))
        return STACK_LEFT_CANARY_SMASH_DETECTED;
    
    if (!check_r_canary(stk))
        return STACK_RIGHT_CANARY_SMASH_DETECTED;

    return STACK_OK;
}


StackStatus stack_init(stack_t *stk, size_t capacity
                       ON_DEBUG(,
                       const char *_dbug_var_name,
                       const char *_dbug_func_name,
                       const char *_dbug_filename,
                       const int _dbug_line)) {
    assert(stk != NULL);
    assert(capacity < (SIZE_MAX - left_cnry_size() - CANARY_SIZE) / sizeof(stk_elem_t));
    ON_DEBUG(
    assert(_dbug_var_name != NULL);
    assert(_dbug_func_name != NULL);
    assert(_dbug_filename != NULL);
    )

    size_t alloc_size = left_cnry_size() + capacity * sizeof(stk_elem_t) +
                        CANARY_SIZE * 2;

    stk->data = calloc(alloc_size, 1);
    if ((stk->data) == NULL)
        return STACK_INIT_ALLOC_ERROR;

    stk->size = 0;
    stk->capacity = capacity;

    ON_DEBUG(
    stk->_dbug_var_name = _dbug_var_name;
    stk->_dbug_func_name = _dbug_func_name;
    stk->_dbug_filename = _dbug_filename;
    stk->_dbug_line = _dbug_line;
    
    set_struct_canaries(stk);
    set_canaries(stk);
    )

    ASSERT_OK(stk);

    return STACK_OK;
}


static StackStatus stack_resize(stack_t *stk, size_t new_capacity) {
    assert(stk != NULL);

    uint8_t *tmp = realloc(stk->data,
                           left_cnry_size() + new_capacity * sizeof(stk_elem_t) + CANARY_SIZE * 2);

    if (tmp == NULL)
        return STACK_REALLOC_FAIL;

    stk->data = tmp;
    stk->capacity = new_capacity;

    return STACK_OK;
}


void stack_push(stack_t *stk, stk_elem_t elem, StackStatus *err) {
    assert(stk != NULL);
    assert(err != NULL);
    ASSERT_OK(stk);

    if (stk->size == stk->capacity) {
        *err = stack_resize(stk, stk->capacity * 2);
        if (*err != STACK_OK)
            return;
    }

    move_right_canary(stk, true);
    *(R_DATA + stk->size) = elem;
    stk->size++;

    *err = STACK_OK;

    ASSERT_OK(stk);
}


stk_elem_t stack_pop(stack_t *stk, StackStatus *err) {
    assert(stk != NULL);
    assert(err != NULL);
    ASSERT_OK(stk);

    if (stk->size == 0) {
        *err = STACK_CANNOT_POP_FROM_EMPTY;
        return 0;
    }

    stk_elem_t last_elem = *(R_DATA + stk->size - 1);
    move_right_canary(stk, false);

    if (stk->size - 1 <= stk->capacity / 4) {
        *err = stack_resize(stk, stk->capacity / 2);
        if (*err != STACK_OK)
            return 0;
    }

    stk->size--;

    ASSERT_OK(stk);

    *err = STACK_OK;

    return last_elem;
}

void stack_destroy(stack_t *stk) {
    assert(stk != NULL);
    
    free(stk->data);
    stk->data = NULL;
}
