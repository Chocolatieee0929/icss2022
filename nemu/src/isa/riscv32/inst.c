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

#include "local-include/reg.h"
#include <common.h>
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

#ifdef CONFIG_FTRACE
void func_trace(Decode *s,word_t target);
#endif

enum {
  TYPE_I, TYPE_U, TYPE_S, TYPE_J, TYPE_R, TYPE_B,
  TYPE_N, // none
};

// src -> R(rs1)
#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12);} while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12) << 12,32);} while(0)

#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)

#define immJ() do { \
       	*imm = SEXT((BITS(i, 31, 31) << 20| BITS(i, 19,12) << 12  | (BITS(i,20,20) <<11)\
			| (BITS(i, 30, 21) << 1)), 21);\
} while(0)
#define immB() do {\
	*imm = SEXT((BITS(i,31, 31) << 12) | (BITS(i, 30, 25) << 5) | (BITS(i,11,8)<<1) | (BITS(i, 7,7) << 11),12);\
}while(0);
 // Put this "Log(ANSI_FG_CYAN "%#x\n" ANSI_NONE, *imm);", if you want to debug 

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  // dest, src1, src2和imm 分别代表目的操作数, 两个源操作数和立即数
  uint32_t i = s->isa.inst.val;
  // BITS 用于位抽取
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    // src1R()和src2R()两个辅助宏,寄存器的读取结果记录到相应的操作数变量中  
    // immI等辅助宏, 用于从指令中抽取出立即数
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J: 		   immJ(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_B: 
		 src1R();
		 if(rs2 != 0)	 src2R(); 
		 // debug
		 //Log(ANSI_FG_CYAN "src1:%#x  src2:%#x\n" ANSI_NONE, *src1,*src2);
		 immB(); break;
  }
}

void csrrwrs(word_t rd, word_t src1, word_t imm, bool tt){
  word_t t, *ptr = &R(0);
  if ( imm == 773 ) {
	 // ptr = &cpu.mtvec;
  }
  else if ( imm == 768 ) {
	 //  ptr = &cpu.mstatus;
  }
  else if ( imm == 833 ) {
	  ptr = &cpu.pc;
  }
  else if ( imm == 834 ) {
	  // ptr = &cpu.mcause;
  }
  t = *ptr;
  if ( tt ) {
	  *ptr = src1;
  } 
  else {
	  *ptr = t | src1;
  }
  R(rd) = t;
}

// 译码(instruction decode, ID)
static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  // INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作)
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (int)((SEXT((long long)src1, 32) * SEXT((long long)src2, 32)) >> 0)); 
  // TYPE_I
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  // 立即数逻辑右移
  INSTPAT("000000? ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> (imm & 0x1f));
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << BITS(imm, 5, 0)); 
  // wrong!!!
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I, csrrwrs(rd, src1, imm, true)); 
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = src1 < imm );
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2),16));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2));
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm );
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, IFDEF(CONFIG_FTRACE,func_trace(s,s->pc+imm));s->dnpc = (src1 + imm) & ~(word_t)1; R(rd) = s->pc + 4);
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1+imm);
  // 立即数算术右移
  INSTPAT("010000? ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (word_t)(((int32_t)src1) >> (imm & 0x1f))); 
  INSTPAT("010000? ????? ????? 101 ????? 00110 11", sraiw  , I, R(rd) = (sword_t)SEXT(src1, 32) >> BITS(imm, 4, 0));
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);


  // TYPE_S
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2 & 0xffff));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2 ));

  // TYPE_J
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, IFDEF(CONFIG_FTRACE,func_trace(s,s->pc+imm));s->dnpc = s->pc; s->dnpc += imm; R(rd) = s->pc + 4);
  
  // TYPE_R
  INSTPAT("0000000 ????? ????? 101 ????? 01110 11", srlw   , R, R(rd) = SEXT(BITS(src1, 31, 0) >> BITS(src2, 4, 0), 32));
  INSTPAT("0100000 ????? ????? 101 ????? 01110 11", sraw   , R, R(rd) = (sword_t)SEXT(src1, 32) >> BITS(src2, 4, 0));
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = (src1 + src2));
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = (src1 - src2));
  INSTPAT("0100000 ????? ????? 000 ????? 01110 11", subw   , R, R(rd) = SEXT(src1 - src2, 32));
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = ((int32_t)src1 >> (src2& 0x1f)));
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = (src1 << src2));
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = (src1 >> (src2& 0x1f)));
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (uint32_t)src1 < (uint32_t)src2 ? 1:0);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) =  (sword_t)src1 < (sword_t)src2); 
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = (uint32_t)src1 / (uint32_t)src2); 
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
  INSTPAT("0000001 ????? ????? 000 ????? 01110 11", mulw   , R, R(rd) = SEXT(src1 * src2, 32));
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , R, R(rd) = ((long long)src1 * (long long)src2) >> 32);
  // INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (int)((SEXT((long long)src1, 32) * SEXT((long long)src2, 32)) >> 32));
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu , R, R(rd) = (((long long)src1 * SEXT((long long)src2, 32)) >> 32));
  INSTPAT("0000000 ????? ????? 001 ????? 01110 11", sllw   , R, R(rd) = SEXT(src1 << BITS(src2, 4, 0), 32));
  // INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = SEXT((SEXT(src1, 32) * SEXT(src2, 32)) ,32)); 
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = (src1 ^ src2));
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = (src1 | src2));  
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (int)src1 % (int)src2);
  //INSTPAT("0000001 ????? ????? 110 ????? 01110 11", remw   , R, R(rd) = SEXT(src1, 32) % SEXT(src2, 32));
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1 % src2);
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = (src1 & src2));
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (word_t)((int32_t)src1 % (int32_t)src2));
  // div rd, rs1, rs2 x[rd] = x[rs1] ÷s x[rs2]
  // 除法(Divide). R-type, RV32M and RV64M.
  // 用寄存器 x[rs1]的值除以寄存器 x[rs2]的值，向零舍入，将这些数视为二进制补码，把商写入 x[rd]
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div   , R, R(rd) = ((int32_t)src1 / (int32_t)src2));

  // TYPE_B
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, if(src1 != src2) s->dnpc = s->pc+imm);
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s->dnpc = (src1 == src2 ? s->pc + imm : s->dnpc));
  // 之前因为没有设置不等于条件，从而导致程序陷入循环
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, s->dnpc = ((int32_t)src1 >= (int32_t)src2 ? s->pc + imm : s->dnpc));
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, s->dnpc = ((uint32_t)src1 >= (uint32_t)src2 ? s->pc + imm : s->dnpc));
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, s->dnpc = ((int32_t)src1 < (int32_t)src2 ? s->pc + imm : s->dnpc));
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, s->dnpc = ((uint32_t)src1 < (uint32_t)src2 ? s->pc + imm : s->dnpc));
  // TYPE_U
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, 0)); // R(10) is $a0
  // 若前面所有的模式匹配规则都无法成功匹配, 则将该指令视为非法指令
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  // 取指令,更新s->snpc
  s->isa.inst.val = inst_fetch(&s->snpc, 4);

  return decode_exec(s);
}
