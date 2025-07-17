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

enum { MODE_U, MODE_S, MODE_M = 3 };
#define CSRS(_) _(mstatus) _(mtvec) _(mepc) _(mcause) _(satp) _(mscratch)
#define CSR_ENUM(x) CSR_##x
#define CSR_ENUM_INIT(x) CSR_ENUM(x),

enum {
  CSRS(CSR_ENUM_INIT)
  NR_CSR
};

extern const word_t csr_addr[NR_CSR];
extern int csr_addr_map[];
void init_csr_addr_map();

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

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

#endif
