#include "secstack.h"

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <malloc.h>

#define ANSI_GREEN "\e[32m"
#define ANSI_RED "\e[31m"
#define ANSI_GREY "\e[90m"
#define ANSI_YELLOW "\e[33m"
#define ANSI_PURPLE "\e[35m"
#define ANSI_LIGHT_BLUE "\e[36m"
#define ANSI_BLUE "\e[34m"
#define ANSI_RESET "\e[0m"

#define ASSERT_OK(stk) do { \
    StackStatus status = stack_verify((stk)); \
    \
    ON_DEBUG( \
        if (status != STACK_OK) { \
            fprintf(stderr, ANSI_RED "[SECSTACK ERROR]" ANSI_RESET ": " ANSI_LIGHT_BLUE "%s\n" ANSI_RESET \
                            "stack " ANSI_BLUE "'%s'" ANSI_GREY " [%p] " ANSI_RESET \
                            "created by " ANSI_GREEN "'%s()'" ANSI_RESET " at " ANSI_GREEN "'%s':%d\n" ANSI_RESET \
                            "==================================================\n" \
                            ANSI_LIGHT_BLUE "capacity" ANSI_RESET " = " ANSI_RED "%zu\n" ANSI_RESET \
                            ANSI_LIGHT_BLUE "size" ANSI_RESET " = " ANSI_RED "%zu\n" ANSI_RESET\
                            ANSI_LIGHT_BLUE "data" ANSI_RESET " = " ANSI_RED "[%p]\n" ANSI_RESET \
                            "==================================================\n", \
                            stack_error_str(status), (stk)->_dbug_var_name, (void *)(stk), \
                            (stk)->_dbug_func_name, (stk)->_dbug_filename, (stk)->_dbug_line, \
                            (stk)->capacity, (stk)->size, (void *)(stk)->data); \
            \
            fprintf(stderr, "Stack data " ANSI_GREY "[%p]" ANSI_RESET ":\n", (void *)((stk)->data)); \
            \
            for (size_t i = 0; i < (stk)->size; i++) { \
                fprintf(stderr, ANSI_LIGHT_BLUE "[%zu]" ANSI_RESET " = " ANSI_RED "<" DBUG_PRINTF_LIT ">\n" ANSI_RESET, i, (stk)->data[i]); \
            } \
            \
            fprintf(stderr, "==================================================\n"); \
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
        case STACK_MAX_SIZE_REACHED:
            return "stack size reached its capacity";
        case STACK_CANNOT_POP_FROM_EMPTY:
            return "cannot pop from empty stack";
        case STACK_SIZE_BIGGER_THAN_CAPACITY:
            return "stack size is bigger than its capacity";
        case STACK_CAPACITY_BIGGER_THAN_MUSABLE:
            return "stack capacity is bigger than size of memory allocated for it";
        case STACK_DATA_NULL_PTR:
            return "pointer to stack data is NULL";
        default:
            return "unknown error";
    }
}


static StackStatus stack_verify(stack_t *stk) {
    if (stk->size > stk->capacity) {
        return STACK_SIZE_BIGGER_THAN_CAPACITY;
    }

    if (stk->data == NULL) {
        return STACK_DATA_NULL_PTR;
    }

    if (stk->capacity > malloc_usable_size(stk->data)) {
        return STACK_CAPACITY_BIGGER_THAN_MUSABLE;
    }

    return STACK_OK;
}


StackStatus stack_init(stack_t *stk, size_t capacity
                       ON_DEBUG(,
                       const char *_dbug_var_name,
                       const char *_dbug_func_name,
                       const char *_dbug_filename,
                       const int _dbug_line)) {
    assert(stk != NULL);
    ON_DEBUG(
    assert(_dbug_var_name != NULL);
    assert(_dbug_func_name != NULL);
    assert(_dbug_filename != NULL);
    )

    stk->data = calloc(capacity, sizeof(stk_elem_t));
    if ((stk->data) == NULL)
        return STACK_INIT_ALLOC_ERROR;

    stk->size = 0;
    stk->capacity = capacity;

    ON_DEBUG(
    stk->_dbug_var_name = _dbug_var_name;
    stk->_dbug_func_name = _dbug_func_name;
    stk->_dbug_filename = _dbug_filename;
    stk->_dbug_line = _dbug_line;
    )

    ASSERT_OK(stk);

    return STACK_OK;
}


void stack_push(stack_t *stk, stk_elem_t elem, StackStatus *err) {
    assert(stk != NULL);
    assert(err != NULL);
    ASSERT_OK(stk);

    if (stk->size == stk->capacity) { // error >
        *err = STACK_MAX_SIZE_REACHED;
        return;
    }

    stk->data[stk->size] = elem;
    stk->size++;

    // fprintf(stderr, "err: [%p]\n", (void *)err);
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

    stk_elem_t last_elem = stk->data[stk->size - 1];
    stk->size--;

    *err = STACK_OK;

    ASSERT_OK(stk);
    return last_elem;
}

void stack_destroy(stack_t *stk) {
    assert(stk != NULL);
    
    free(stk->data);
    stk->data = NULL;
}
