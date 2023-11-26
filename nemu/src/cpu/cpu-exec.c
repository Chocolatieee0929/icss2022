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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <common.h>
#include <monitor/watchpoint.h>
#include <string.h>
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#ifdef CONFIG_FTRACE
void func_printf();
#endif

#define MAX_INST_TO_PRINT 10
#define IRING_BUF_SIZE 16 // 环形缓冲区大小
#define IRING_BUF_PC_START_INDEX 3 // 存放指令信息的开始位置
static char iringbuf[IRING_BUF_SIZE][128+IRING_BUF_PC_START_INDEX]; // 环形缓冲区 
static size_t iringbuf_head = 0; // 当前指令占据的位置

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
static uint64_t g_timer = 0; // unit: us
static bool g_print_step = false;

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
  strcpy(iringbuf[iringbuf_head] + IRING_BUF_PC_START_INDEX, _this->logbuf);
  iringbuf_head = (iringbuf_head + 1)% IRING_BUF_SIZE;
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  // Judge watchpoints' difference
#ifdef CONFIG_WATCHPOINT
  if(is_wps_diff()){
	Log("nemu: STOP at pc = %x\n" ,nemu_state.halt_pc);
  	nemu_state.state = NEMU_STOP;
	// puts("Some watchpoints have been changed.\n");
  }
#endif
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
}

static void exec_once(Decode *s, vaddr_t pc) {
  // 让CPU执行当前PC指向的一条指令
  s->pc = pc;
  // static next PC
  s->snpc = pc;
  isa_exec_once(s);
  // dynamic next PC, 更新PC
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;   // the length of this pc.
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst.val;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);   // 每字节按照两位十六进制输出
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;
  //strcpy(iringbuf[iringbuf_head] + IRING_BUF_PC_START_INDEX, s->logbuf);
  //iringbuf_head = (iringbuf_head + 1)% IRING_BUF_SIZE;

#ifndef CONFIG_ISA_loongarch32r
  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst.val, ilen);
#else
  p[0] = '\0'; // the upstream llvm does not support loongarch32r
#endif
#endif
}

static void execute(uint64_t n) {
  
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    // 用于记录客户指令的计数器加1
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

void print_iring_buf(){
  const char point[IRING_BUF_PC_START_INDEX] = "-->";
  for(int i = 0; i < IRING_BUF_SIZE; ++i){
	if((iringbuf[i][IRING_BUF_PC_START_INDEX] == '\0')) break;
	if(((i+1) % IRING_BUF_SIZE) == iringbuf_head){
	   // snprintf 函数，它会限制复制的字符数，并且可以防止缓冲区溢出
	   int len = strlen(iringbuf[i]);
	   char temp[len];
	   //snprintf(iringbuf[i], IRING_BUF_PC_START_INDEX+1, "%s", point);
	   strncpy(temp, point, IRING_BUF_PC_START_INDEX+1);
	   strncat(temp, iringbuf[i] + 3, sizeof(temp) - strlen(temp));
	   snprintf(iringbuf[i], len+1, "%s", temp);
	}
#ifdef CONFIG_ITRACE_COND
	if(ITRACE_COND) { log_write("%s\n", iringbuf[i]); }
#endif
	printf("%s\n", iringbuf[i]);
  }
  return;
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  // initialize iringbuf
  iringbuf_head = 0;
  for (int i = 0; i < IRING_BUF_SIZE; ++i) {
	memset(iringbuf[i], ' ', IRING_BUF_PC_START_INDEX);  // 前几个位置初始
	iringbuf[i][IRING_BUF_PC_START_INDEX] = '\0';
  }
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; break;
    case NEMU_END: case NEMU_ABORT:
      //if(nemu_state.state == NEMU_END && nemu_state.halt_ret != 0){
	print_iring_buf();
     // }
      IFDEF(CONFIG_FTRACE, func_printf());
      Log("nemu: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
  }
}
