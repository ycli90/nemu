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
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#include <memory/paddr.h>
#include <cpu/cpu.h>

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  for (int i = 0; i < RISCV_GPR_NUM; i++) {
    if (cpu.gpr[i] != ref_r->gpr[i]) {
      printf("difftest failed, gpr[%d] ref: " FMT_WORD ", dut: " FMT_WORD "\n", i, ref_r->gpr[i], cpu.gpr[i]);
      return false;
    }
  }
  if (cpu.pc != ref_r->pc) {
    printf("difftest failed, pc ref: " FMT_WORD ", dut: " FMT_WORD "\n", ref_r->pc, cpu.pc);
    return false;
  }
  return true;
}

void isa_difftest_attach() {
  const word_t R = 1;
  vaddr_t saved_pc = cpu.pc;
  word_t saved_reg = cpu.gpr[R];
  vaddr_t code_addr = cpu.pc;
  word_t saved_mem[NR_CSR];
  for (int i = 0; i < NR_CSR; i++) {
    saved_mem[i] = paddr_read(code_addr + 4 * i, 4);
  }
  vaddr_t code_pc = code_addr;
  for (int i = 0; i < NR_CSR; i++, code_pc += 4) {
    word_t code;
    cpu.gpr[R] = cpu.csr[i];
    code = (csr_addr[i] << 20) | (R << 15) | 0x00001073;
    cpu.pc = code_pc;
    paddr_write(code_pc, 4, code);
    ref_difftest_memcpy(code_pc, guest_to_host(code_pc), 4, DIFFTEST_TO_REF);
    ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
    ref_difftest_exec(1);
  }
  cpu.pc = saved_pc;
  cpu.gpr[R] = saved_reg;
  for (int i = 0; i < NR_CSR; i++) {
    paddr_write(code_addr + 4 * i, 4, saved_mem[i]);
  }
  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
  ref_difftest_memcpy(code_addr, guest_to_host(code_addr), 4 * NR_CSR, DIFFTEST_TO_REF);
}
