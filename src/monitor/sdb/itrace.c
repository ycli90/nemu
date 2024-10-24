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

#include <monitor/sdb.h>

#define INST_HISTORY_SIZE 100
char inst_history[INST_HISTORY_SIZE][128];
int inst_history_count = 0;
int inst_history_current_index = 0;

void inst_history_add(const char *s) {
  strncpy(inst_history[inst_history_current_index], s, sizeof(inst_history[0]));
  inst_history_current_index = (inst_history_current_index + 1) % INST_HISTORY_SIZE;
  inst_history_count += 1;
  Assert(inst_history_count >= 0, "instruction trace count overflow");
}

void inst_history_print() {
  int ind = inst_history_count < INST_HISTORY_SIZE ? 0 : inst_history_current_index;
  int n = inst_history_count < INST_HISTORY_SIZE ? inst_history_current_index : INST_HISTORY_SIZE;
  for (int i = 0; i < n; ++i) {
    printf("%s\n", inst_history[ind]);
    ind = (ind + 1) % INST_HISTORY_SIZE;
  }
}
