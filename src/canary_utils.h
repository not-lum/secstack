#pragma once

#include "secstack.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdalign.h>


static size_t left_cnry_size() {
    size_t alignment = alignof(stk_elem_t);
    if (CANARY_SIZE % alignment == 0)
        return CANARY_SIZE;
    else
        return CANARY_SIZE + alignment - (CANARY_SIZE % alignment);
}


static void move_right_canary(stack_t *stk, bool to_right) {
    uint8_t *src = stk->data + left_cnry_size() + stk->size * sizeof(stk_elem_t);
    uint8_t *dest = (to_right) ? (src + sizeof(stk_elem_t)) : (src - sizeof(stk_elem_t));

    memmove(dest, src, CANARY_SIZE);
}


static void guard_alignment(uint8_t *addr) {
    for (size_t i = CANARY_SIZE; i < left_cnry_size(); i++) {
        *(addr + i) = ALIGNMENT_FILL;
    }
}

