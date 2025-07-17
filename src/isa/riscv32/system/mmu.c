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
#include <memory/vaddr.h>
#include <memory/paddr.h>

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  word_t satp = cpu.csr[CSR_satp];
  if ((satp >> 31) == 1) return MMU_TRANSLATE;
  else return MMU_DIRECT;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  if ((vaddr >> PAGE_SHIFT) != ((vaddr + len - 1) >> PAGE_SHIFT)) {
    panic("memory access cross page");
  }
  word_t satp = cpu.csr[CSR_satp];
  paddr_t page_dir_addr = satp << 12;
  paddr_t page_dir_entry_paddr = page_dir_addr + sizeof(uint32_t) * (vaddr >> 22);
  word_t page_dir_entry = paddr_read(page_dir_entry_paddr, 4);
  if ((page_dir_entry & 0x1) == 0) {
    printf("isa_mmu_translate error at pc 0x%x, vaddr = 0x%x, as.ptr = 0x%x\n", cpu.pc, vaddr, page_dir_addr);
  }
  assert((page_dir_entry & 0x1) == 0x1);
  assert((page_dir_entry & 0xf) == 0x1);
  paddr_t page_table_addr = page_dir_entry >> 10 << 12;
  paddr_t page_table_entry_paddr = page_table_addr + sizeof(uint32_t) * ((vaddr >> 12) & ((1 << 10) - 1));
  word_t page_table_entry = paddr_read(page_table_entry_paddr, 4);
  // assert((page_table_entry & 0x1) == 0x1);
  if (type == MEM_TYPE_IFETCH) {
    assert((page_table_entry & 0x9) == 0x9);
  } else if (type == MEM_TYPE_READ) {
    assert((page_table_entry & 0x3) == 0x3);
  } else if (type == MEM_TYPE_WRITE) {
    assert((page_table_entry & 0x7) == 0x7);
  } else {
    panic("invalid type: %d", type);
  }
  paddr_t page_addr = page_table_entry >> 10 << 12;
  paddr_t paddr = page_addr | (vaddr & PAGE_MASK);
  return paddr;
}
