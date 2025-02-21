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

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>
#include <isa.h>

extern const char *csr_name[NR_CSR];
extern const word_t csr_addr[NR_CSR];

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

static inline int check_csr_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < NR_CSR));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])
#define csr(idx) (cpu.csr[check_csr_idx(idx)])

static inline word_t csr_lookup_by_addr(word_t addr) {
  for (word_t i = 0; i < NR_CSR; ++i) {
    if (csr_addr[i] == addr) return i;
  }
  panic("unsupported csr address 0x%x\n", addr);
}

static inline word_t csr_lookup_by_name(const char *name) {
  for (word_t i = 0; i < NR_CSR; ++i) {
    if (strcmp(csr_name[i], name) == 0) return i;
  }
  panic("unsupported csr name %s\n", name);
}

static inline word_t get_csr_by_address(word_t addr) {
  return cpu.csr[csr_lookup_by_addr(addr)];
}

static inline void set_csr_by_address(word_t addr, word_t val) {
  cpu.csr[csr_lookup_by_addr(addr)] = val;
}

static inline word_t* p_csr_by_address(word_t addr) {
  return &cpu.csr[csr_lookup_by_addr(addr)];
}

static inline word_t get_csr_by_name(const char *name) {
  return cpu.csr[csr_lookup_by_name(name)];
}

static inline void set_csr_by_name(const char *name, word_t val) {
  cpu.csr[csr_lookup_by_name(name)] = val;
}

static inline word_t* p_csr_by_name(const char *name) {
  return &cpu.csr[csr_lookup_by_name(name)];
}

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

#endif
