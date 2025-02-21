/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
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

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>
#include <macro.h>
#include <generated/autoconf.h>

word_t expr(char *e, bool *success);

// checkpoint
void init_wp_pool();
bool add_wp(char *s_expr);
bool del_wp(int n);
void display_wp();
bool check_wp();

// instruction trace ring buffer
void inst_history_add(const char *);
void inst_history_print();

// function trace
#define SYMBOL_NAME_MAX_LEN 128
struct function_info {
    bool is_function;
    char name[SYMBOL_NAME_MAX_LEN];
    word_t start_address;
    word_t end_address;
};
void register_function(bool is_function, const char *name, word_t start_address, word_t end_address);
void print_function_info();
int search_function(word_t address);

enum function_trace_type { FUNCTION_CALL, FUNCTION_RETURN };
struct function_trace_item {
    enum function_trace_type type;
    word_t pc;
    int current_function_index;
    word_t target_address;
    int target_function_index;
    int level;
};

void add_function_trace(word_t pc, word_t address, enum function_trace_type type);
void print_function_trace();
void print_function_stack();
void function_stack_save(FILE *fp);
void function_stack_load(FILE *fp);
#endif
