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
#include "../local-include/reg.h"
#define IRQ_TIMER 0x80000007

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  IFDEF(CONFIG_ETRACE, log_write("[etrace] interrupt from pc " FMT_PADDR ", mcause: " FMT_WORD "\n", epc, NO));
  cpu.csr[CSR_mepc] = epc;
  cpu.csr[CSR_mcause] = NO;
  // set mstatus.MPP to cpu.mode and enter M mode
  cpu.csr[CSR_mstatus] &= ~(0x3 << 11);
  cpu.csr[CSR_mstatus] |= (cpu.mode & 0x3) << 11;
  cpu.mode = MODE_M;
  // set mstatus.MPIE to mstatus.MIE and disable interrupt
  cpu.csr[CSR_mstatus] &= ~(0x1 << 7);
  cpu.csr[CSR_mstatus] |= (cpu.csr[CSR_mstatus] >> 3 & 0x1) << 7;
  cpu.csr[CSR_mstatus] &= ~(0x1 << 3);
  return cpu.csr[CSR_mtvec];
}

word_t isa_query_intr() {
  if (cpu.INTR && cpu.csr[CSR_mstatus] >> 3 & 0x1) {
    cpu.INTR = false;
    return IRQ_TIMER;
  }
  return INTR_EMPTY;
}

vaddr_t isa_intr_ret() {
  // recover mode from mstatus.MPP and set mstatus.MPP to U
  cpu.mode = (cpu.csr[CSR_mstatus] >> 11) & 0x3;
  cpu.csr[CSR_mstatus] &= ~(0x3 << 11);
  cpu.csr[CSR_mstatus] |= (MODE_U & 0x3) << 11;
  // recover mstatus.MIE from mstatus.MPIE and set mstatus.MPIE to 1
  cpu.csr[CSR_mstatus] &= ~(0x1 << 3);
  cpu.csr[CSR_mstatus] |= (cpu.csr[CSR_mstatus] >> 7 & 0x1) << 3;
  cpu.csr[CSR_mstatus] |= 0x1 << 7;
  return cpu.csr[CSR_mepc];
}
