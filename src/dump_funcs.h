#pragma once

#include "secstack.h"
#include "canary_utils.h"

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <malloc.h>

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

void dump_canary(stack_t *stk, StackStatus status, bool left, FILE *out, bool colors);
void dump_stack_to(stack_t *stk, StackStatus status, FILE *out, bool colors);
void dump_stack(stack_t *stk, StackStatus status);
