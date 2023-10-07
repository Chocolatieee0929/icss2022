/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <generated/autoconf.h>
#include <isa.h>
#include <locale.h>



#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif
#ifdef CONFIG_MTRACE_COND
#define MTRACE_SIZE 128
static char mtrace[MTRACE_SIZE];
static const char mtrace_type[2][3] = {"Mw","Mr"};
#endif
#ifdef CONFIG_MTRACE
static void memory_trace(paddr_t addr, int len, int data, int flag){
  strncpy(mtrace, mtrace_type[flag], 2);
  memset(mtrace + 2, ' ', 4);
  char *p = mtrace + 6;
  p += snprintf(p, sizeof(mtrace)-6, FMT_WORD "    ", addr);
  p += snprintf(p, mtrace + sizeof(mtrace)-p, "%2d""    ", len);
  uint8_t *inst = (uint8_t *)&data; 
  for (int i = len - 1; i >= 0; i--) {
	p += snprintf(p, 4, " %02x", inst[i]);
  }
  *p = '\0';
#ifdef CONFIG_MTRACE_COND
  // if(MTRACE_COND) {
     log_write("%s\n", mtrace); 
  //}
#endif
  puts(mtrace);
}
#endif

// 将要访问的内存地址映射到pmem中的相应偏移位置
uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len){
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr) {
  panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
      addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
}

void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
#ifdef CONFIG_MEM_RANDOM
  uint32_t *p = (uint32_t *)pmem;
  int i;
  for (i = 0; i < (int) (CONFIG_MSIZE / sizeof(p[0])); i ++) {
    p[i] = rand();
  }
#endif
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) {
#ifdef CONFIG_MTRACE
	word_t data = pmem_read(addr, len);
	memory_trace(addr, len, data, 1);
	return data;
#else
	return pmem_read(addr, len);
#endif
  }
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len));
#ifdef CONFIG_MTRACE
	memory_trace(addr, len, -1, 1);
#endif
  out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE
	memory_trace(addr, len, data, 0);
#endif
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  out_of_bound(addr);
}
