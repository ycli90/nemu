/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <monitor/sdb.h>

#define FUNCTION_LIST_SIZE 2000
#define FUNCTION_STACK_SIZE 100
#define FUNCTION_TRACE_SIZE 100
#define FUNCTION_CALL_STRING "Call <%s @%#8x> from <%s>\n"
#define FUNCTION_RET_STRING "Ret  <%s> to <%s @%#8x>\n"
struct function_info function_list[FUNCTION_LIST_SIZE];
static int n_function = 0;
struct function_trace_item function_trace[FUNCTION_TRACE_SIZE];
int n_function_trace = 0;
int function_trace_current_index = 0;
int function_stack[FUNCTION_STACK_SIZE];
int n_function_stack = 0;

void register_function(const char *name, word_t start_address, word_t end_address) {
  Assert(n_function < FUNCTION_LIST_SIZE, "function info list full");
  function_list[n_function].start_address = start_address;
  function_list[n_function].end_address = end_address;
  strcpy(function_list[n_function].name, name);
  ++n_function;
}

void print_function_info() {
  for (int i = 0; i < n_function; ++i) {
    struct function_info *fip = function_list + i;
    log_write("0x%8x - 0x%8x  %s\n", fip->start_address, fip->end_address, fip->name);
  }
}

int search_function(word_t address) {
  for (int i = 0; i < n_function; ++i) {
    struct function_info *fip = function_list + i;
    if (address >= fip->start_address && address < fip->end_address) {
      return i;
    }
  }
  panic("unknown function at address %x", address);
  return -1;
}

void add_function_trace(word_t pc, word_t target_address, enum function_trace_type type) {
  int current_function_index = search_function(pc);
  int target_function_index = search_function(target_address);
  struct function_trace_item *ftip = &function_trace[function_trace_current_index];
  ftip->type = type;
  ftip->pc = pc;
  ftip->current_function_index = current_function_index;
  ftip->target_address = target_address;
  ftip->target_function_index = target_function_index;
  if (type == FUNCTION_CALL) {
    ftip->level = n_function_stack;
    Assert(target_address == function_list[target_function_index].start_address, "Call function address not equal to function start address");
    Assert(n_function_stack < FUNCTION_STACK_SIZE, "function stack full");
    function_stack[n_function_stack++] = target_function_index;
  } else {
    if (n_function_stack == 0) panic("return from empty function stack");
    --n_function_stack;
    ftip->level = n_function_stack;
  }
  if (type== FUNCTION_CALL) {
    log_write("[ftrace] %#8x: " FUNCTION_CALL_STRING, pc, function_list[target_function_index].name, target_address, function_list[current_function_index].name);
  } else {
    log_write("[ftrace] %#8x: " FUNCTION_RET_STRING, pc, function_list[current_function_index].name, function_list[target_function_index].name, target_address);
  }
  function_trace_current_index = (function_trace_current_index + 1) % FUNCTION_TRACE_SIZE;
  ++n_function_trace;
  Assert(n_function_trace >= 0, "function trace count overflow");
}

void print_function_trace() {
  int ind = n_function_trace < FUNCTION_TRACE_SIZE ? 0 : function_trace_current_index;
  int n = n_function_trace < FUNCTION_TRACE_SIZE ? function_trace_current_index : FUNCTION_TRACE_SIZE;
  for (int i = 0; i < n; ++i) {
    struct function_trace_item *ftip = &function_trace[ind];
    printf("%#8x: ", ftip->pc);
    for (int j = 0; j < 2 * ftip->level; ++j) putchar(' ');
    if (ftip->type== FUNCTION_CALL) {
      printf(FUNCTION_CALL_STRING, function_list[ftip->target_function_index].name, ftip->target_address, function_list[ftip->current_function_index].name);
    } else {
      printf(FUNCTION_RET_STRING, function_list[ftip->current_function_index].name, function_list[ftip->target_function_index].name, ftip->target_address);
    }
    ind = (ind + 1) % FUNCTION_TRACE_SIZE;
  }
}

void print_function_stack() {
  for (int i = 0; i < n_function_stack; ++i) {
    printf("0x%8x %s\n", function_list[function_stack[i]].start_address, function_list[function_stack[i]].name);
  }
}