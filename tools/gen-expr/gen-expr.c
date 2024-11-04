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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define MAX_BUF_LEN 65536

// this should be enough
static char buf[MAX_BUF_LEN] = {};
static char code_buf[MAX_BUF_LEN + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int level = 0;
static int p = 0;
const char * ops[] = {"+", "-", "*", "/"};

static inline int is_enough_space() {
  return MAX_BUF_LEN - p >= 12 * level + 11 + 1;
}

void gen(const char *str) {
  int len = strlen(str);
  if (p + len < MAX_BUF_LEN) {
    strcpy(buf + p, str);
    p += len;
  } else {
    printf("gen expression too long\n");
  }
}

void gen_num() {
  char temp_str[16];
  int n = rand();
  sprintf(temp_str, "%du", n);
  gen(temp_str);
}

void gen_rand_op() {
  int i = rand() % 4;
  gen(ops[i]);
}

void gen_blank() {
  if (!is_enough_space()) return;
  switch (rand() % 3) {
    case 0: gen(" "); break;
    default: break;
  }
}

static void gen_rand_expr() {
  level++;
  gen_blank();
  int n = is_enough_space() ? 3 : 1;
  switch (rand() % n) {
    case 0: gen_num(); break;
    case 1: gen("("); gen_rand_expr(); gen(")"); break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
  gen_blank();
  level--;
  buf[p] = '\0';
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    if (i % 100 == 0) fprintf(stderr, "generated %d\n", i);
    assert(level == 0);
    p = 0;
    gen_rand_expr();
    assert(p < MAX_BUF_LEN);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    if (ret == -1) continue;

    char *pc = buf;
    while (*pc) {
      if (*pc == 'u') *pc = ' ';
      pc++;
    }
    printf("%u %s\n", result, buf);
  }
  return 0;
}
