#include "secstack.h"

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdalign.h>

#define ANSI_GREEN "\e[32m"
#define ANSI_RED "\e[31m"
#define ANSI_GREY "\e[90m"
#define ANSI_YELLOW "\e[33m"
#define ANSI_PURPLE "\e[35m"
#define ANSI_LIGHT_BLUE "\e[36m"
#define ANSI_BLUE "\e[34m"

#define ANSI_BG_CYAN "\e[46m"
#define ANSI_BG_WHITE "\e[47m"
#define ANSI_BG_YELLOW "\e[43m"
#define ANSI_BG_RED "\e[41m"

#define ANSI_RESET "\e[0m"

// 0xB07CEBAC0CE7B0BE
const uint8_t L_CANARY[8] = {0xB0, 0x7C, 0xEB, 0xAC, 0x0C, 0xE7, 0xB0, 0xBE};
// 0xABBA3EC0BA3EBAE7
const uint8_t R_CANARY[8] = {0xAB, 0xBA, 0x3E, 0xC0, 0xBA, 0x3E, 0xBA, 0xE7};

#define ALIGNMENT_FILL 0x67
#define CANARY_SIZE 8

#define R_DATA (stk_elem_t *)(stk->data + left_cnry_size())


// todo: dump to file
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

static size_t left_cnry_size() {
    // (CANARY_SIZE + x) % alignment = 0
    // => x = -CANARY_SIZE (mod alignment)

    size_t alignment = alignof(stk_elem_t);
    if (CANARY_SIZE % alignment == 0)
        return CANARY_SIZE;
    else
        return CANARY_SIZE + alignment - (CANARY_SIZE % alignment);
}

static void mem_hex(uint8_t *data, size_t data_size, size_t str_size, char *out_str) {
    assert(data != NULL);
    assert(out_str != NULL);

    size_t i = 0;
    size_t len = (data_size <= (str_size - 1) / 2) ? (data_size) : ((str_size - 1) / 2); 

    for (; i < len; i++) {
        sprintf(out_str + i * 2, "%02X", data[i]);
    }
    
    *((char *)(out_str) + i * 2) = '\0';
}

void dump_canary(stack_t *stk, StackStatus status, bool left) {
    assert(stk != NULL);

    const char *ind_color = ANSI_PURPLE;
    const char *val_color = ANSI_GREEN;
    const char *arrow_str = (left) ? (ANSI_PURPLE "   <-- LEFT CANARY") : (ANSI_PURPLE "   <-- RIGHT CANARY");

    char hexed_canary[left_cnry_size() * 2 + 1] = {};
    uint8_t *cnry_addr = (left) ? (stk->data) :
                         (stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t));
    size_t cnry_size = (left) ? left_cnry_size() : CANARY_SIZE;

    mem_hex(cnry_addr, cnry_size, left_cnry_size() * 2 + 1, hexed_canary);

    if (status == STACK_LEFT_CANARY_SMASH_DETECTED && left) {
        ind_color = ANSI_RED;
        val_color = ANSI_YELLOW ANSI_BG_RED;
        arrow_str = ANSI_RED "   <-- LEFT CANARY SMASHED";
    } else if (status == STACK_RIGHT_CANARY_SMASH_DETECTED && !left) {
        ind_color = ANSI_RED;
        val_color = ANSI_YELLOW ANSI_BG_RED;
        arrow_str = ANSI_RED "   <-- RIGHT CANARY SMASHED";
    }
    fprintf(stderr, "%s[%p]" ANSI_RESET 
                    " = %s<0x%s>" ANSI_RESET
                    "%s\n" ANSI_RESET,
                    ind_color, (void *)cnry_addr, val_color, hexed_canary, arrow_str);
}

void dump_stack(stack_t *stk, StackStatus status) {
    assert(stk != NULL);

    if (status == STACK_OK) {
        fprintf(stderr, ANSI_YELLOW "\n[SECSTACK DUMP]\n" ANSI_RESET);
    } else {
        fprintf(stderr, ANSI_RED "\n[SECSTACK ERROR]" ANSI_RESET ": (" ANSI_LIGHT_BLUE "%s" ANSI_RESET ")\n",
                stack_error_str(status));
    }

    fprintf(stderr, "stack " ANSI_BLUE "'%s'" ANSI_GREY " [%p] " ANSI_RESET
                    "created by " ANSI_GREEN "'%s()'" ANSI_RESET " at " ANSI_GREEN "'%s':%d\n" ANSI_RESET
                    "==================================================\n"
                    ANSI_LIGHT_BLUE "capacity" ANSI_RESET " = " ANSI_YELLOW "%zu\n" ANSI_RESET
                    ANSI_LIGHT_BLUE "size" ANSI_RESET " = " ANSI_YELLOW "%zu\n" ANSI_RESET
                    ANSI_LIGHT_BLUE "data" ANSI_RESET " = " ANSI_YELLOW "[%p]\n" ANSI_RESET
                    "==================================================\n",
                    stk->_dbug_var_name, (void *)(stk),
                    stk->_dbug_func_name, stk->_dbug_filename, stk->_dbug_line,
                    stk->capacity, stk->size, (void *)stk->data);
    
    fprintf(stderr, "Stack elements data " ANSI_GREY "[%p]" ANSI_RESET ":\n", (void *)(stk->data));
    dump_canary(stk, status, true);
    
    for (size_t i = 0; i < stk->size; i++) {
        const char *begin_str = "* " ANSI_LIGHT_BLUE;
        const char *end_str = "";

        if (i == stk->size - 1) {
            begin_str = "* " ANSI_GREEN;
            end_str = ANSI_BLUE "    <-- LAST STACK ELEMENT";
        }
    
        fprintf(stderr, "%s[%zu]" ANSI_RESET 
                        " = " ANSI_YELLOW "<" DBUG_PRINTF_LIT ">" ANSI_RESET
                        "%s\n" ANSI_RESET,
                        begin_str, i, *(R_DATA + i), end_str);
    }

    dump_canary(stk, status, false);

    fprintf(stderr, "==================================================\n");
    size_t full_data_size = malloc_usable_size(stk->data);

    fprintf(stderr, "Full data buffer hex dump (" ANSI_LIGHT_BLUE "%zu bytes" ANSI_RESET ") "
            ANSI_GREY "[%p]" ANSI_RESET ":\n",
            full_data_size, stk->data);

    bool new_line = false;

    for (size_t i = 0; i < full_data_size - 1; i += 2) {
        new_line = false;
        if (i % 16 == 0) {
            new_line = true;
            fprintf(stderr, "\n" ANSI_GREY "[%p] " ANSI_RESET, stk->data + i);
        }
        
        const char *first_b_col = "";
        const char *second_b_col = "";
        if (i < stk->size * sizeof(stk_elem_t) + CANARY_SIZE + left_cnry_size()) {
            first_b_col = ANSI_YELLOW;
        }
        if (i + 1 < stk->size * sizeof(stk_elem_t) + CANARY_SIZE + left_cnry_size()) {
            second_b_col = ANSI_YELLOW;
        }
        
        fprintf(stderr, "%s%02X%s%02X " ANSI_RESET, first_b_col, stk->data[i],
                                         second_b_col, stk->data[i + 1]);
    }

    if (full_data_size % 2 != 0)
        fprintf(stderr, "%02X\n", stk->data[full_data_size - 1]);
    else if (!new_line)
        fprintf(stderr, "\n");

    fprintf(stderr, "\n==================================================\n");

    if (status == STACK_OK) {
        fprintf(stderr, ANSI_YELLOW "[END SECSTACK DUMP]\n\n" ANSI_RESET);
    } else {
        fprintf(stderr, ANSI_RED "[END SECSTACK ERROR]\n\n" ANSI_RESET);
    }
}


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
        default:
            return "unknown error";
    }
}


StackStatus canary_verify(stack_t *stk) {
    assert(stk != NULL);

    for (size_t i = 0; i < left_cnry_size(); i++) {
        if ((i < CANARY_SIZE && stk->data[i] != *((uint8_t *)&L_CANARY + i)) ||
            (i >= CANARY_SIZE && stk->data[i] != ALIGNMENT_FILL))
            return STACK_LEFT_CANARY_SMASH_DETECTED;
    }

    for (size_t i = 0; i < CANARY_SIZE; i++) {
        if (stk->data[left_cnry_size() + stk->size * sizeof(stk_elem_t) + i] != *((uint8_t *)&R_CANARY + i))
            return STACK_RIGHT_CANARY_SMASH_DETECTED;
    }
    

    return STACK_OK;
}

StackStatus stack_verify(stack_t *stk) {
    if (stk->size > stk->capacity) {
        return STACK_SIZE_BIGGER_THAN_CAPACITY;
    }

    if (stk->data == NULL) {
        return STACK_DATA_NULL_PTR;
    }

    if (stk->capacity * sizeof(stk_elem_t) + CANARY_SIZE + left_cnry_size() > malloc_usable_size(stk->data)) {
        return STACK_CAPACITY_BIGGER_THAN_MUSABLE;
    }

    return canary_verify(stk);
}


static void guard_alignment(uint8_t *addr) {
    for (size_t i = CANARY_SIZE; i < left_cnry_size(); i++) {
        *(addr + i) = ALIGNMENT_FILL;
    }
}

StackStatus stack_init(stack_t *stk, size_t capacity
                       ON_DEBUG(,
                       const char *_dbug_var_name,
                       const char *_dbug_func_name,
                       const char *_dbug_filename,
                       const int _dbug_line)) {
    assert(stk != NULL);
    size_t l_cnry_size = left_cnry_size();
    assert(capacity < (SIZE_MAX - l_cnry_size - CANARY_SIZE) / sizeof(stk_elem_t));
    ON_DEBUG(
    assert(_dbug_var_name != NULL);
    assert(_dbug_func_name != NULL);
    assert(_dbug_filename != NULL);
    )

    size_t alloc_size = l_cnry_size + capacity * sizeof(stk_elem_t) + CANARY_SIZE;

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
    )

    memcpy(stk->data, &L_CANARY, CANARY_SIZE);
    guard_alignment(stk->data);
    memcpy(stk->data + l_cnry_size, &R_CANARY, CANARY_SIZE);

    ASSERT_OK(stk);

    return STACK_OK;
}

static void move_right_canary(stack_t *stk, bool to_right) {
    uint8_t *src = stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t);
    uint8_t *dest = (to_right) ? (src + sizeof(stk_elem_t)) : (src - sizeof(stk_elem_t));

    memmove(dest, src, CANARY_SIZE);
}

static StackStatus stack_resize(stack_t *stk, size_t new_capacity) {
    assert(stk != NULL);

    uint8_t *tmp = realloc(stk->data,
                           left_cnry_size() + new_capacity * sizeof(stk_elem_t) + CANARY_SIZE);

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
