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

paddr_t vaddr_to_paddr(vaddr_t vaddr, int len, int type) {
  int mmu_check_ret = isa_mmu_check(vaddr, len, type);
  if (mmu_check_ret == MMU_TRANSLATE) {
    paddr_t paddr = isa_mmu_translate(vaddr, len, type);
    return paddr;
  } else if (mmu_check_ret == MMU_DIRECT) {
    return vaddr;
  } else {
    panic("vaddr_to_paddr failed");
  }
}

word_t vaddr_ifetch(vaddr_t addr, int len) {
  paddr_t paddr = vaddr_to_paddr(addr, len, MEM_TYPE_IFETCH);
  return paddr_read(paddr, len);
}

word_t vaddr_read(vaddr_t addr, int len) {
  paddr_t paddr = vaddr_to_paddr(addr, len, MEM_TYPE_READ);
  return paddr_read(paddr, len);
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
  paddr_t paddr = vaddr_to_paddr(addr, len, MEM_TYPE_WRITE);
  return paddr_write(paddr, len, data);
}
