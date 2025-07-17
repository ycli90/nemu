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

#include <device/map.h>
#include <memory/paddr.h>

enum {
  reg_disk_present,
  reg_disk_blksz,
  reg_disk_blkcnt,
  reg_disk_io_buf,
  reg_disk_io_blkno,
  reg_disk_io_blkcnt,
  reg_disk_io_cmd,
  nr_reg
};

#define OFFSET(reg) (sizeof(uint32_t) * reg)
#define BLKSZ 512
static uint32_t *disk_base = NULL;
static FILE *fp = NULL;

void do_disk_io() {
  if (fp) {
    fseek(fp, disk_base[reg_disk_io_blkno] * BLKSZ, SEEK_SET);
    void *host_addr = guest_to_host(disk_base[reg_disk_io_buf]);
    size_t len = disk_base[reg_disk_io_blkcnt] * BLKSZ;
    int ret;
    if (disk_base[reg_disk_io_cmd] == 1) {
      ret = fread(host_addr, len, 1, fp);
    } else if (disk_base[reg_disk_io_cmd] == 2) {
      ret = fwrite(host_addr, len, 1, fp);
    } else {
      panic("invalid disk io cmd: %d", disk_base[reg_disk_io_cmd]);
    }
    assert(ret == 1);
  }
}

static void disk_io_handler(uint32_t offset, int len, bool is_write) {
  if (is_write) {
    assert(offset < OFFSET(nr_reg) && offset >= OFFSET(reg_disk_io_buf));
    if (offset == OFFSET(reg_disk_io_cmd)) {
      do_disk_io();
      disk_base[reg_disk_io_cmd] = 0;
    }
  } else {
    assert(offset <= OFFSET(reg_disk_blkcnt));
  }
}
  
void init_disk() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  disk_base = (uint32_t *)new_space(space_size);
  const char *diskimg = getenv("diskimg");
  if (diskimg) {
    fp = fopen(diskimg, "r+");
    if (fp) {
      fseek(fp, 0, SEEK_END);
      disk_base[reg_disk_blkcnt] = (ftell(fp) + BLKSZ - 1) / BLKSZ;
      rewind(fp);
    }
  }
  disk_base[reg_disk_present] = fp != NULL;
  disk_base[reg_disk_blksz] = BLKSZ;
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("disk", CONFIG_DISK_CTL_PORT, disk_base, space_size, disk_io_handler);
#else
  add_mmio_map("disk", CONFIG_DISK_CTL_MMIO, disk_base, space_size, disk_io_handler);
#endif
}
