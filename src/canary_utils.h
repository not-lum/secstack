#pragma once

#include "secstack.h"
#include <stddef.h>
#include <stdint.h>

size_t left_cnry_size();
uint8_t *get_aligned_addr(uint8_t *addr, size_t alignment);
void guard_alignment(uint8_t *addr, size_t padding_size);
bool check_l_canary(stack_t *stk);
uint8_t *get_r_canary_addr(stack_t *stk);
bool check_r_canary(stack_t *stk);
void move_right_canary(stack_t *stk, bool to_right);
void set_canaries(stack_t *stk);
void set_struct_canaries(stack_t *stk);
bool check_struct_l_canary(stack_t *stk);
bool check_struct_r_canary(stack_t *stk);
