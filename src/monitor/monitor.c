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
#include <memory/paddr.h>
#include <monitor/sdb.h>
#include <elf.h>

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm();

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static char *elf_files = NULL;
static int difftest_port = 1234;

static long load_img() {
  if (img_file == NULL || strlen(img_file) == 0) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}

static bool parse_elf(const char *elf_file) {
  Log("Parse ELF file %s", elf_file);
  FILE *fp = fopen(elf_file, "rb");
  Assert(fp, "Can not open '%s'", elf_file);

  size_t __attribute__((unused)) n_read;

  // read ELF header
  Elf32_Ehdr ehdr;
  n_read = fread(&ehdr, sizeof(ehdr), 1, fp);
  if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
    ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
    ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
    ehdr.e_ident[EI_MAG3] != ELFMAG3) {
    Log("%s is not a valid ELF file.", elf_file);
    fclose(fp);
    return false;
  }

  // scan section headers
  Elf32_Off symtab_offset = 0;
  Elf32_Word symtab_size = 0;
  Elf32_Word strtab_index = 0;
  for (int i = 0; i < ehdr.e_shnum; ++i) {
    fseek(fp, ehdr.e_shoff + i * ehdr.e_shentsize, SEEK_SET);
    Elf32_Shdr shdr;
    n_read = fread(&shdr, sizeof(shdr), 1, fp);
    if (shdr.sh_type == SHT_SYMTAB) {
      symtab_offset = shdr.sh_offset;
      symtab_size = shdr.sh_size;
      strtab_index = shdr.sh_link;
      break;
    }
  }

  if (symtab_offset != 0 && symtab_size != 0) {
    fseek(fp, ehdr.e_shoff + strtab_index * ehdr.e_shentsize, SEEK_SET);
    Elf32_Shdr shdr;
    n_read = fread(&shdr, sizeof(shdr), 1, fp);
    Elf32_Off strtab_offset = shdr.sh_offset;
    // parse symbol table
    for (Elf32_Word i = 0; i < symtab_size / sizeof(Elf32_Sym); ++i) {
      fseek(fp, symtab_offset + i * sizeof(Elf32_Sym), SEEK_SET);
      Elf32_Sym sym;
      n_read = fread(&sym, sizeof(sym), 1, fp);
      if (ELF32_ST_TYPE(sym.st_info) != STT_FUNC) continue;
      // read symbol name from strtab
      char *sym_name = (char *)malloc(SYMBOL_NAME_MAX_LEN);
      sym_name[SYMBOL_NAME_MAX_LEN - 1] = 0;
      fseek(fp, strtab_offset + sym.st_name, SEEK_SET);
      int n = 0;
      while (n < SYMBOL_NAME_MAX_LEN - 1) {
        char ch;
        n_read = fread(&ch, 1, 1, fp);
        sym_name[n] = ch;
        if (ch == '\0') break;
        ++n;
      }
      register_function(ELF32_ST_TYPE(sym.st_info) == STT_FUNC, sym_name, sym.st_value, sym.st_value + sym.st_size);
      free(sym_name);
    }
  } else {
      Log("Symbol table not found.");
      fclose(fp);
      return false;
  }

  fclose(fp);
  return true;
}

static void parse_elfs() {
  if (elf_files == NULL || strlen(elf_files) == 0) {
    Log("No ELF file is given.");
    return;
  }
  const char *delim = ",";
  char *elf_file = strtok(elf_files, delim);
  while (elf_file != NULL) {
    parse_elf(elf_file);
    elf_file = strtok(NULL, delim);
  }
  print_function_info();
}

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {"img"      , required_argument, NULL,  1 },
    {"elf"      , required_argument, NULL,  2 },
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "bhl:d:p:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; break;
      case 2: elf_files = optarg; break;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t--img=IMAGE_FILE        load image file\n");
        printf("\t--elf=ELF_FILE        load ELF file\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Parse function name and address from ELF file.*/
  parse_elfs();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

  IFDEF(CONFIG_ITRACE, init_disasm());

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
