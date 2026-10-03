#include "dump_funcs.h"
#include "secstack.h"
#include "canary_utils.h"

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <malloc.h>
#include <inttypes.h>


ON_DEBUG(
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

void set_struct_cnry_format(const char **name_col, const char **val_col,
                            const char **end_str, bool colors, bool left) {
    *name_col = COLOR_IF(colors, ANSI_RED);
    *val_col = COLOR_IF(colors, ANSI_BG_RED ANSI_YELLOW);

    if (left)
        *end_str = ANSI_RESET "   " ANSI_RED "<-- STRUCT LEFT CANARY SMASHED" ANSI_RESET;
    else
        *end_str = ANSI_RESET "   " ANSI_RED "<-- STRUCT RIGHT CANARY SMASHED" ANSI_RESET;
    

    if (!colors)
        *end_str = (left) ? "   <-- STRUCT LEFT CANARY SMASHED" : "   <-- STRUCT RIGHT CANARY SMASHED";
}

void dump_canary(stack_t *stk, StackStatus status, bool left, FILE *out, bool colors) {
    assert(stk != NULL);
    assert(out != NULL);

    const char *ind_color = COLOR_IF(colors, ANSI_PURPLE);
    const char *val_color = COLOR_IF(colors, ANSI_GREEN);
    const char *arrow_color = COLOR_IF(colors, ANSI_PURPLE);
    const char *arrow_str = (left) ? "   <-- LEFT CANARY" : "   <-- RIGHT CANARY";

    char hexed_canary[left_cnry_size() * 2 + 1] = {};
    uint8_t *cnry_addr = (left) ? (stk->data) : (get_r_canary_addr(stk));
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
    assert(out != NULL);
    if (status == STACK_NULL) return;

    if (status == STACK_OK) {
        fprintf(out, "%s\n[SECSTACK DUMP]\n%s",
                COLOR_IF(colors, ANSI_YELLOW), COLOR_IF(colors, ANSI_RESET));
    } else {
        fprintf(out, "%s\n[SECSTACK ERROR]%s: (%s%s%s)\n",
                COLOR_IF(colors, ANSI_RED), COLOR_IF(colors, ANSI_RESET),
                COLOR_IF(colors, ANSI_LIGHT_BLUE), stack_error_str(status),
                COLOR_IF(colors, ANSI_RESET));
    }

    const char *l_cnry_name_col = ANSI_PURPLE;
    const char *l_cnry_val_col = ANSI_GREEN;
    const char *l_cnry_end_str = ANSI_RESET;

    const char *r_cnry_name_col = ANSI_PURPLE;
    const char *r_cnry_val_col = ANSI_GREEN;
    const char *r_cnry_end_str = ANSI_RESET;

    if (status == STACK_STRUCT_LEFT_CANARY_SMASH_DETECTED) {
        set_struct_cnry_format(&l_cnry_name_col, &l_cnry_val_col,
                               &l_cnry_end_str, colors, true);
    }

    if (status == STACK_STRUCT_RIGHT_CANARY_SMASH_DETECTED) {
        set_struct_cnry_format(&r_cnry_name_col, &r_cnry_val_col,
                               &r_cnry_end_str, colors, false);
    }

    fprintf(out, "stack %s'%s'%s [%p] %s"
                    "created by %s'%s()'%s at %s'%s':%d\n%s"
                    "==================================================\n"
                    "%sl_canary%s = %s<%" S_CNRY_PRINTF_LIT ">%s\n"
                    "%scapacity%s = %s%zu\n%s"
                    "%ssize%s = %s%zu\n%s"
                    "%sdata%s = %s[%p]\n%s"
                    "%sr_canary%s = %s<%" S_CNRY_PRINTF_LIT ">%s\n"
                    "==================================================\n",
                    COLOR_IF(colors, ANSI_BLUE), stk->_dbug_var_name,
                    COLOR_IF(colors, ANSI_GREY), (void *)(stk), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_GREEN), stk->_dbug_func_name, COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_GREEN), stk->_dbug_filename, stk->_dbug_line,
                    COLOR_IF(colors, ANSI_RESET),
                    l_cnry_name_col, COLOR_IF(colors, ANSI_RESET),
                    l_cnry_val_col, stk->l_canary, l_cnry_end_str,
                    COLOR_IF(colors, ANSI_LIGHT_BLUE), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_YELLOW), stk->capacity, COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_LIGHT_BLUE), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_YELLOW), stk->size, COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_LIGHT_BLUE), COLOR_IF(colors, ANSI_RESET),
                    COLOR_IF(colors, ANSI_YELLOW), (void *)stk->data, COLOR_IF(colors, ANSI_RESET),
                    r_cnry_name_col, COLOR_IF(colors, ANSI_RESET),
                    r_cnry_val_col, stk->r_canary, r_cnry_end_str);
    
    if (stk->data != NULL) {
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
            size_t valuable_part_len = get_r_canary_addr(stk) + CANARY_SIZE - stk->data;

            if (i < valuable_part_len) {
                first_b_col = COLOR_IF(colors, ANSI_YELLOW);
            }
            if (i + 1 < valuable_part_len) {
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
    }

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
)
