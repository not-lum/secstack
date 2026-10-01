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

#define COLOR_IF(enabled, color) ((enabled) ? (color) : "")

// 0xB07CEBAC0CE7B0BE
const uint8_t L_CANARY[8] = {0xB0, 0x7C, 0xEB, 0xAC, 0x0C, 0xE7, 0xB0, 0xBE};
// 0xABBA3EC0BA3EBAE7
const uint8_t R_CANARY[8] = {0xAB, 0xBA, 0x3E, 0xC0, 0xBA, 0x3E, 0xBA, 0xE7};

#define ALIGNMENT_FILL 0x67
#define CANARY_SIZE 8

#define R_DATA (stk_elem_t *)(stk->data + left_cnry_size())

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

void dump_canary(stack_t *stk, StackStatus status, bool left, FILE *out, bool colors) {
    assert(stk != NULL);
    assert(out != NULL);

    const char *ind_color = COLOR_IF(colors, ANSI_PURPLE);
    const char *val_color = COLOR_IF(colors, ANSI_GREEN);
    const char *arrow_color = COLOR_IF(colors, ANSI_PURPLE);
    const char *arrow_str = (left) ? "   <-- LEFT CANARY" : "   <-- RIGHT CANARY";

    char hexed_canary[left_cnry_size() * 2 + 1] = {};
    uint8_t *cnry_addr = (left) ? (stk->data) :
                         (stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t));
    size_t cnry_size = (left) ? left_cnry_size() : CANARY_SIZE;

    mem_hex(cnry_addr, cnry_size, left_cnry_size() * 2 + 1, hexed_canary);

    if (status == STACK_LEFT_CANARY_SMASH_DETECTED && left) {
        ind_color = COLOR_IF(colors, ANSI_RED);
        val_color = COLOR_IF(colors, ANSI_YELLOW ANSI_BG_RED);
        arrow_color = COLOR_IF(colors, ANSI_RED);
        arrow_str = "   <-- LEFT CANARY SMASHED";
    } else if (status == STACK_RIGHT_CANARY_SMASH_DETECTED && !left) {
        ind_color = COLOR_IF(colors, ANSI_RED);
        val_color = COLOR_IF(colors, ANSI_YELLOW ANSI_BG_RED);
        arrow_color = COLOR_IF(colors, ANSI_RED);
        arrow_str = "   <-- RIGHT CANARY SMASHED";
    }
    fprintf(out, "%s[%p]%s"
                 " = %s<0x%s>%s"
                 "%s%s\n%s",
                 ind_color, (void *)cnry_addr, COLOR_IF(colors, ANSI_RESET),
                 val_color, hexed_canary, COLOR_IF(colors, ANSI_RESET),
                 arrow_color, arrow_str, COLOR_IF(colors, ANSI_RESET));
}

void dump_stack_to(stack_t *stk, StackStatus status, FILE *out, bool colors) {
    assert(stk != NULL);
    assert(out != NULL);

    if (status == STACK_OK) {
        fprintf(out, "%s\n[SECSTACK DUMP]\n%s",
                COLOR_IF(colors, ANSI_YELLOW), COLOR_IF(colors, ANSI_RESET));
    } else {
        fprintf(out, "%s\n[SECSTACK ERROR]%s: (%s%s%s)\n",
                COLOR_IF(colors, ANSI_RED), COLOR_IF(colors, ANSI_RESET),
                COLOR_IF(colors, ANSI_LIGHT_BLUE), stack_error_str(status),
                COLOR_IF(colors, ANSI_RESET));
    }

    fprintf(out, "stack %s'%s'%s [%p] %s"
                    "created by %s'%s()'%s at %s'%s':%d\n%s"
                    "==================================================\n"
                    "%scapacity%s = %s%zu\n%s"
                    "%ssize%s = %s%zu\n%s"
                    "%sdata%s = %s[%p]\n%s"
                    "==================================================\n",
                    COLOR_IF(colors, ANSI_BLUE), stk->_dbug_var_name,
                    COLOR_IF(colors, ANSI_GREY), (void *)(stk), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_GREEN), stk->_dbug_func_name, COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_GREEN), stk->_dbug_filename, stk->_dbug_line,
                    COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_LIGHT_BLUE), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_YELLOW), stk->capacity, COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_LIGHT_BLUE), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_YELLOW), stk->size, COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_LIGHT_BLUE), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_YELLOW), (void *)stk->data, COLOR_IF(colors, ANSI_RESET));
    
    fprintf(out, "Stack elements data %s[%p]%s:\n",
            COLOR_IF(colors, ANSI_GREY), (void *)(stk->data), COLOR_IF(colors, ANSI_RESET));
    dump_canary(stk, status, true, out, colors);
    
    for (size_t i = 0; i < stk->size; i++) {
        const char *begin_str = colors ? "* " ANSI_LIGHT_BLUE : "* ";
        const char *end_str = "";

        if (i == stk->size - 1) {
            begin_str = colors ? "* " ANSI_GREEN : "* ";
            end_str = colors ? ANSI_BLUE "    <-- LAST STACK ELEMENT" : "    <-- LAST STACK ELEMENT";
        }
    
        fprintf(out, "%s[%zu]%s"
                     " = %s<" DBUG_PRINTF_LIT ">%s"
                     "%s\n%s",
                     begin_str, i, COLOR_IF(colors, ANSI_RESET),
                     COLOR_IF(colors, ANSI_YELLOW), *(R_DATA + i), COLOR_IF(colors, ANSI_RESET),
                     end_str, COLOR_IF(colors, ANSI_RESET));
    }

    dump_canary(stk, status, false, out, colors);

    fprintf(out, "==================================================\n");
    size_t full_data_size = malloc_usable_size(stk->data);

    fprintf(out, "Full data buffer hex dump (%s%zu bytes%s) %s[%p]%s:\n",
            COLOR_IF(colors, ANSI_LIGHT_BLUE), full_data_size, COLOR_IF(colors, ANSI_RESET),
            COLOR_IF(colors, ANSI_GREY), stk->data, COLOR_IF(colors, ANSI_RESET));

    bool new_line = false;

    for (size_t i = 0; i < full_data_size - 1; i += 2) {
        new_line = false;
        if (i % 16 == 0) {
            new_line = true;
            fprintf(out, "\n%s[%p] %s",
                    COLOR_IF(colors, ANSI_GREY), stk->data + i, COLOR_IF(colors, ANSI_RESET));
        }
        
        const char *first_b_col = "";
        const char *second_b_col = "";
        if (i < stk->size * sizeof(stk_elem_t) + CANARY_SIZE + left_cnry_size()) {
            first_b_col = COLOR_IF(colors, ANSI_YELLOW);
        }
        if (i + 1 < stk->size * sizeof(stk_elem_t) + CANARY_SIZE + left_cnry_size()) {
            second_b_col = COLOR_IF(colors, ANSI_YELLOW);
        }
        
        fprintf(out, "%s%02X%s%02X %s", first_b_col, stk->data[i],
                                         second_b_col, stk->data[i + 1],
                                         COLOR_IF(colors, ANSI_RESET));
    }

    if (full_data_size % 2 != 0)
        fprintf(out, "%02X\n", stk->data[full_data_size - 1]);
    else if (!new_line)
        fprintf(out, "\n");

    fprintf(out, "\n==================================================\n");

    if (status == STACK_OK) {
        fprintf(out, "%s[END SECSTACK DUMP]\n\n%s",
                COLOR_IF(colors, ANSI_YELLOW), COLOR_IF(colors, ANSI_RESET));
    } else {
        fprintf(out, "%s[END SECSTACK ERROR]\n\n%s",
                COLOR_IF(colors, ANSI_RED), COLOR_IF(colors, ANSI_RESET));
    }
}

void dump_stack(stack_t *stk, StackStatus status) {
    dump_stack_to(stk, status, stderr, true);
    FILE* log_file = fopen(".secstack.log", "a");
    if (log_file != NULL) {
        setvbuf(log_file, NULL, _IONBF, 0);
        dump_stack_to(stk, status, log_file, false);

        fclose(log_file);
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
