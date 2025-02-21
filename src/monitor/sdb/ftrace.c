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

#define FUNCTION_LIST_SIZE 3000
#define FUNCTION_STACK_SIZE 100
#define FUNCTION_TRACE_SIZE 100
#define FUNCTION_CALL_STRING "Call %#8x <%s> from <%s>\n"
#define FUNCTION_RET_STRING "Ret  <%s> to <%s @%#8x>\n"
struct function_info function_list[FUNCTION_LIST_SIZE];
static int n_function = 0;
struct function_trace_item function_trace[FUNCTION_TRACE_SIZE];
int n_function_trace = 0;
int function_trace_current_index = 0;
struct function_trace_item function_stack[FUNCTION_STACK_SIZE];
int n_function_stack = 0;

void register_function(bool is_function, const char *name, word_t start_address, word_t end_address) {
  Assert(n_function < FUNCTION_LIST_SIZE, "function info list full");
  function_list[n_function].is_function = is_function;
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
    if ((fip->is_function && address >= fip->start_address && address < fip->end_address)
      || (!fip->is_function && address == fip->start_address)) {
      return i;
    }
  }
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
    if (target_function_index != -1) {
      Assert(target_address == function_list[target_function_index].start_address, "Call function address not equal to function start address");
    }
    Assert(n_function_stack < FUNCTION_STACK_SIZE, "function stack full");
    function_stack[n_function_stack++] = *ftip;
  } else {
    if (n_function_stack == 0) panic("return from empty function stack");
    --n_function_stack;
    ftip->level = n_function_stack;
  }
  log_write("[ftrace]: %s %#8x <%s> from %#8x <%s>\n", ftip->type == FUNCTION_CALL ? "Call" : "Ret ",
    ftip->target_address, ftip->target_function_index  >= 0 ? function_list[ftip->target_function_index ].name : "",
    ftip->pc,             ftip->current_function_index >= 0 ? function_list[ftip->current_function_index].name : "");
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
    printf("%s %#8x <%s> from %#8x <%s>\n", ftip->type == FUNCTION_CALL ? "Call" : "Ret ",
      ftip->target_address, ftip->target_function_index  >= 0 ? function_list[ftip->target_function_index ].name : "",
      ftip->pc,             ftip->current_function_index >= 0 ? function_list[ftip->current_function_index].name : "");
    ind = (ind + 1) % FUNCTION_TRACE_SIZE;
  }
}

void print_function_stack() {
  for (int i = 0; i < n_function_stack; ++i) {
    printf("Call %#8x <%s> from %#8x <%s>\n",
      function_stack[i].target_address, function_stack[i].target_function_index  >= 0 ? function_list[function_stack[i].target_function_index ].name : "",
      function_stack[i].pc,             function_stack[i].current_function_index >= 0 ? function_list[function_stack[i].current_function_index].name : "");
  }
}

void function_stack_save(FILE *fp) {
  fwrite(&n_function_stack, sizeof(int), 1, fp);
  fwrite(function_stack, sizeof(struct function_trace_item), n_function_stack, fp);
}

void function_stack_load(FILE *fp) {
  size_t ret = fread(&n_function_stack, sizeof(int), 1, fp);
  if (ret != 1) {
    printf("read n_function_stack failed, ret = %lu\n", ret);
    return;
  }
  ret = fread(function_stack, sizeof(struct function_trace_item), n_function_stack, fp);
  if (ret != n_function_stack) {
    printf("read function_stack failed, ret = %lu\n", ret);
    return;
  }
}
