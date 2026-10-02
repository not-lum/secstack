
#include "secstack.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdalign.h>

// 0xB07CEBAC0CE7B0BE
const uint8_t L_CANARY[8] = {0xB0, 0x7C, 0xEB, 0xAC, 0x0C, 0xE7, 0xB0, 0xBE};
// 0xABBA3EC0BA3EBAE7
const uint8_t R_CANARY[8] = {0xAB, 0xBA, 0x3E, 0xC0, 0xBA, 0x3E, 0xBA, 0xE7};

size_t left_cnry_size() {
    size_t alignment = alignof(stk_elem_t);
    if (CANARY_SIZE % alignment == 0)
        return CANARY_SIZE;
    else
        return CANARY_SIZE + alignment - (CANARY_SIZE % alignment);
}

uint8_t *get_aligned_addr(uint8_t *addr, size_t alignment) {
    // dest + x % CANARY_SIZE = 0
    // => x = CANARY_SIZE - dest (mod CANARY_SIZE)

    size_t remainder = (size_t)addr % alignment;
    
    if (remainder == 0)
        return addr;
    else
        return addr + CANARY_SIZE - remainder;
}


void guard_alignment(uint8_t *addr, size_t padding_size) {
    for (size_t i = 0; i < padding_size; i++) {
        *(addr + i) = ALIGNMENT_FILL;
    }
}


bool check_l_canary(stack_t *stk) {
    for (size_t i = 0; i < left_cnry_size(); i++) {
        if ((i < CANARY_SIZE && stk->data[i] != L_CANARY[i]) ||
            (i >= CANARY_SIZE && stk->data[i] != ALIGNMENT_FILL))
            return false;
    }

    return true;
}


uint8_t *get_r_canary_addr(stack_t *stk) {
    return get_aligned_addr(stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t), CANARY_SIZE);
}


bool check_r_canary(stack_t *stk) {
    uint8_t *r_canary_addr = get_r_canary_addr(stk);
    uint8_t *r_canary_padd_addr = stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t);

    for (size_t i = 0; i < CANARY_SIZE; i++) {
        if (*(r_canary_addr + i) != R_CANARY[i]) {
            return false;
        }
    }

    for (size_t i = 0; i < r_canary_addr - r_canary_padd_addr; i++) {
        if (*(r_canary_padd_addr + i) != ALIGNMENT_FILL) {
            return false;
        }
    }

    return true;
}


void move_right_canary(stack_t *stk, bool to_right) {
    uint8_t *src = get_r_canary_addr(stk);
    uint8_t *last_elem_addr = stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t);
    uint8_t *dest = (to_right) ? (src + sizeof(stk_elem_t)) : (last_elem_addr - sizeof(stk_elem_t));

    if ((src <= last_elem_addr && to_right) || !to_right) {
        uint8_t *aligned_dest = get_aligned_addr(dest, CANARY_SIZE);
    
        memmove(aligned_dest, src, CANARY_SIZE);
        guard_alignment(dest, aligned_dest - dest);
    }
}


void set_canaries(stack_t *stk) {
    memcpy(stk->data, &L_CANARY, CANARY_SIZE);
    guard_alignment(stk->data + CANARY_SIZE, left_cnry_size() - CANARY_SIZE);
    memcpy(get_r_canary_addr(stk), &R_CANARY, CANARY_SIZE);
}
