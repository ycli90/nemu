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

#include <monitor/sdb.h>

#define NR_WP 32

typedef struct watchpoint {
  int idx; // for debug
  int NO;
  char expr[256];
  word_t value;
  struct watchpoint *next;
  struct watchpoint *prev;
  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
static int wp_seq = 0;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].idx = i;
    wp_pool[i].NO = 0;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].prev = (i == 0 ? NULL : &wp_pool[i - 1]);
  }

  head = NULL;
  free_ = wp_pool;
  wp_seq = 0;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  if (free_ == NULL) return NULL;
  WP *new_wp = free_;
  free_ = free_->next;
  if (free_ != NULL) {
    free_->prev = NULL;
  }
  new_wp->next = NULL;
  new_wp->NO = ++wp_seq;
  if (head == NULL) {
    new_wp->prev = NULL;
    head = new_wp;
    return new_wp;
  }
  WP *p_wp = head;
  while (p_wp->next != NULL) {
    p_wp = p_wp->next;
  }
  p_wp->next = new_wp;
  new_wp->prev = p_wp;
  return new_wp;
}

void free_wp(WP *wp) {
  if (wp->next) wp->next->prev = wp->prev;
  if (wp->prev) wp->prev->next = wp->next;
  if (wp == head) head = wp->next;
  if (free_ != NULL) {
    free_->prev = wp;
  }
  wp->next = free_;
  wp->prev = NULL;
  free_ = wp;
}

bool add_wp(char *s_expr) {
  bool success = true;
  word_t value = expr(s_expr, &success);
  if (!success) {
    printf("invalid expression: %s\n", s_expr);
    return false;
  }
  WP *p_wp = new_wp();
  if (p_wp == NULL) {
    printf("watchpoint pool full\n");
    return false;
  }
  strcpy(p_wp->expr, s_expr);
  p_wp->value = value;
  printf("watchpoint No.%d \"%s\" = " FMT_WORD "\n", p_wp->NO, p_wp->expr, p_wp->value);
  return true;
}

bool del_wp(int n) {
  for (WP *p_wp = head; p_wp != NULL; p_wp = p_wp->next) {
    if (p_wp->NO == n) {
      free_wp(p_wp);
      return true;
    }
  }
  printf("watchpoint %d does not exist\n", n);
  return false;
}

void display_wp() {
  printf("Watchpoints\n");
  for (WP *p = head; p != NULL; p = p->next) {
    printf("No.%d \"%s\" = " FMT_WORD "\n", p->NO, p->expr, p->value);
  }
}

bool check_wp() {
  bool change = false;
  for (WP *p = head; p != NULL; p = p->next) {
    bool success = true;
    word_t value = expr(p->expr, &success);
    if (value != p->value) {
      printf("watchpoint No.%d \"%s\" " FMT_WORD " -> " FMT_WORD "\n", p->NO, p->expr, p->value, value);
      p->value = value;
      change = true;
    }
  }
  return change;
}
