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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <monitor/sdb.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <cpu/difftest.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  if (nemu_state.state != NEMU_END && nemu_state.state != NEMU_ABORT) {
    nemu_state.state = NEMU_QUIT;
  }
  return -1;
}

static int cmd_si(char *args) {
  uint64_t step = 1;
  if (args != NULL) {
    char *str = strtok(args, " ");
    int n = atoi(str);
    if (n == 0) {
      printf("si parse error\n");
      return 0;
    } else {
      if (strtok(NULL, " ") != NULL) {
        printf("si parse error\n");
        return 0;
      }
      step = n;
    }
  }
  printf("exec %lu steps\n", step);
  cpu_exec(step);
  return 0;
}

static int cmd_info(char *args) {
  char *str = strtok(args, " ");
  if (str == NULL || strtok(NULL, " ") != NULL) {
    printf("cmd info parse error\n");
    return 0;
  }
  if (strcmp(str, "r") == 0) {
    isa_reg_display();
  } else if (strcmp(str, "w") == 0) {
    display_wp();
  } else {
    printf("unsupported subcmd %s\n", str);
    return 0;
  }
  return 0;
}

static int cmd_p(char *args) {
  bool success = true;
  word_t result = expr(args, &success);
  if (success) {
    printf("result: " FMT_WORD "(hex) %d(dec) %u(unsigned dec)\n", result, result, result);
  } else {
    printf("eval failed\n");
  }
  return 0;
}

void exam_memory(vaddr_t address, int n) {
  while (n > 0) {
    printf(FMT_PADDR ":", address);
    for (int i = 0; i < 8 && n > 0; i++, n--) {
      printf("  0x%02" PRIx8, vaddr_read(address + i, 1));
    }
    printf("\n");
    address += 8;
  }
}

static int cmd_x(char *args) {
  if (args == NULL) {
    printf("format: x N <expr>\n");
    return 0;
  }
  char *args_end = args + strlen(args);
  char *str = strtok(args, " ");
  if (str == NULL) {
    printf("command x miss argument N\n");
    return 0;
  }
  int n = atoi(str);
  if (n == 0) {
    printf("parse N error\n");
    return 0;
  }
  str += strlen(str) + 1;
  if (str > args_end) {
    printf("command x miss start address\n");
    return 0;
  }
  bool success = true;
  word_t address = expr(str, &success);
  if (!success) {
    printf("address expression error\n");
    return 0;
  }
  printf("cmd x %d " FMT_PADDR "\n", n, address);
  exam_memory(address, n);
  return 0;
}

static int cmd_w(char *args) {
  add_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  if (args == NULL) {
    printf("format: d N\n");
    return 0;
  }
  int n = atoi(args);
  if (n == 0) {
    printf("invalid watchpoint ID\n");
    return 0;
  }
  del_wp(n);
  return 0;
}

static int cmd_itrace(char *args) {
  inst_history_print();
  return 0;
}

static int cmd_ftrace(char *args) {
  print_function_trace();
  return 0;
}

static int cmd_fstack(char *args) {
  print_function_stack();
  return 0;
}

static int cmd_detach(char *args) {
  difftest_detach();
  return 0;
}

static int cmd_attach(char *args) {
  difftest_attach();
  return 0;
}

static int cmd_save(char *args) {
  if (args == NULL) {
    printf("need args\n");
    return 0;
  }
  char *file_name = strtok(args, " ");
  if (file_name == NULL) {
    printf("need file path\n");
    return 0;
  }
  FILE *fp = fopen(file_name, "wb");
  if (fp == NULL) {
    printf("cannot open file %s\n", file_name);
    return 0;
  }
  fwrite(&cpu, sizeof(cpu), 1, fp);
  fwrite(guest_to_host(CONFIG_MBASE), 1, CONFIG_MSIZE, fp);
  function_stack_save(fp);
  fclose(fp);
  return 0;
}

static int cmd_load(char *args) {
  if (args == NULL) {
    printf("need args\n");
    return 0;
  }
  char *file_name = strtok(args, " ");
  if (file_name == NULL) {
    printf("need file path\n");
    return 0;
  }
  FILE *fp = fopen(file_name, "rb");
  if (fp == NULL) {
    printf("cannot open file %s\n", file_name);
    return 0;
  }
  size_t ret = fread(&cpu, sizeof(cpu), 1, fp);
  if (ret < 1) {
    printf("read cpu failed, ret = %lu\n", ret);
    return 0;
  }
  ret = fread(guest_to_host(CONFIG_MBASE), 1, CONFIG_MSIZE, fp);
  if (ret < CONFIG_MSIZE) {
    printf("read mem failed, ret = %lu\n", ret);
    return 0;
  }
  function_stack_load(fp);
  fclose(fp);
  difftest_load();
  return 0;
}

static int cmd_test_expr(char *args) {
  char file_name[256];
  sscanf(args, "%s", file_name);
  FILE *fp = fopen(file_name, "r");
  if (fp == NULL) {
    printf("test file not found\n");
    return 1;
  }
  word_t ref_result;
  char *buf = malloc(65536);
  int total = 0, passed = 0, failed = 0;
  while (fscanf(fp, "%u%[^\n]s", &ref_result, buf) == 2) {
    total++;
    _Log(ANSI_FMT("case #%d\n", ANSI_FG_CYAN), total);
    bool success = true;
    word_t result = expr(buf, &success);
    if (success) {
      if (result == ref_result) {
        printf("test passed. ");
        fprintf(stderr, "test %d\n", total);
        passed++;
      } else {
        printf("test failed. ");
        fprintf(stderr, "expr: %s, result=%u, ref=%u\n", buf, result, ref_result);
        failed++;
      }
      printf("result=%u ref_result=%u\n", result, ref_result);
    } else {
      printf("eval failed\n");
    }
  }
  fclose(fp);
  free(buf);
  printf("total = %d, passed = %d, failed = %d\n", total, passed, failed);
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Single step N times", cmd_si },
  { "info", "Show info", cmd_info },
  { "p", "Evaluate expression", cmd_p },
  { "x", "Show memory", cmd_x },
  { "w", "Set watch point", cmd_w },
  { "d", "Delete watch point", cmd_d },
  { "itrace", "Print instruction trace", cmd_itrace },
  { "ftrace", "Print function trace", cmd_ftrace },
  { "fstack", "Print function stack", cmd_fstack },
  { "detach", "Disable difftest", cmd_detach},
  { "attach", "Enable difftest", cmd_attach},
  { "save", "Save snapshot", cmd_save},
  { "load", "Load from snapshot", cmd_load},
  { "test_expr", "Test expression evaluation", cmd_test_expr },

  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
