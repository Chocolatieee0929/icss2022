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

//#ifdef CONFIG_FTRACE
#include <elf.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <common.h>

#define FUNCNAME_SIZE 32 
#define Func_num 128 

typedef struct{
	  char func_name[FUNCNAME_SIZE];
	    paddr_t func_start;
	      size_t func_size;
}FuncInfo;

FuncInfo Func[Func_num];
void init_elf(const char *elf_file);

//#endif


FILE *elf_fp = NULL;

static Elf32_Ehdr read_system_tab(FILE *fp){
  char headbuf[EI_NIDENT] = {0};
  rewind(fp);
  assert(fread(headbuf, sizeof(char), EI_NIDENT, fp));
  assert(headbuf[0] !=0x7f && headbuf[1] != 'E' && headbuf[2] != 'L' && headbuf[3] != 'F');
  rewind(fp);
  Elf32_Ehdr elfHeader32;
  assert(fread(&elfHeader32, sizeof(Elf32_Ehdr), 1, fp));
  rewind(fp);
  return elfHeader32;
}

int read_elf_symtab(Elf32_Sym *elf_symtab, Elf32_Shdr sec_sym, FILE *fp) {
  int symnum = sec_sym.sh_size / sec_sym.sh_entsize;
  if((sec_sym.sh_size % sec_sym.sh_entsize) != 0) return 0;

  Elf32_Sym elf_func[symnum];
  rewind(fp);
  fseek(fp, sec_sym.sh_offset, SEEK_SET);
  int funcnum = 0;
  for(int i = 0; i < symnum; i++){
	Elf32_Sym symentry ;
	assert(fread(&symentry, sizeof(Elf32_Sym),1,fp));
	if(symentry.st_info == STT_FUNC){
		elf_func[funcnum++] = symentry;	
	}
  }
  if(funcnum != 0) elf_symtab = elf_func;
  rewind(fp);
  return funcnum;
}

// 从符号表中type为FUNC类型将其转换成我们定义的结构体，函数名需要根据字符串表进行读取
int Convert_FuncInfo(Elf32_Sym *elf_symtab, int symnum, Elf32_Shdr elfstrtab,FILE * fp){
	size_t index = 0;
	for(; index < symnum; index++){
		Elf32_Sym symentry = elf_symtab[index];
		fseek(fp, elfstrtab.sh_offset + symentry.st_name * sizeof(char), SEEK_SET);
		assert(fscanf(fp, "%63s", Func[index].func_name));
		Func[index].func_start = symentry.st_value;
		Func[index].func_size  = symentry.st_size;
		// debug
		printf("funcname: %s\tfunc_start = 0x%x\t func_offset = 0x%lx\n", 
				Func[Func_num].func_name, Func[Func_num].func_start,
				Func[index].func_size);
		index ++;
	}
	rewind(fp);
	Func[index].func_name[0] = '\0';
	return 0;
}

// 阅读elf的函数表
void read_elf_func(Elf32_Ehdr Ehdr, FILE *fp){
  size_t shnum = Ehdr.e_shnum;
  int shoff = Ehdr.e_shoff;

  // read the section headers
  Elf32_Shdr elf_shstrtab;
  Elf32_Sym *elf_symtab = NULL;
  size_t symnum = 0;
  Elf32_Shdr elf_strtab;
  
  // 从Section Header里读出 section name strtab
  fseek(fp, shoff + Ehdr.e_shstrndx * Ehdr.e_shentsize, SEEK_SET);
  assert(fread(&elf_shstrtab, sizeof(Elf32_Shdr), 1, fp)); 
  rewind(fp);

  // 从Section Header里读出字符串表和符号表
  fseek(fp, shoff, SEEK_SET);
  for(size_t i = 0; i < shnum; i++){
	Elf32_Shdr secEnt;
	assert(fread(&secEnt, sizeof(Elf32_Shdr), 1, fp));
	if(secEnt.sh_type == SHT_SYMTAB){
		symnum = read_elf_symtab(elf_symtab, secEnt, fp);
		assert(symnum);
	}
	else if(secEnt.sh_type == SHT_STRTAB && i != shoff){
		elf_strtab = secEnt;
	}
  }
  
  // 从符号表中type为FUNC类型将其转换成我们定义的结构体，函数名需要根据字符串表进行读取
  assert(Convert_FuncInfo(elf_symtab, symnum, elf_strtab, fp)>0);
  rewind(fp);
  fclose(fp);
}

// 初始化形成elf文件
void init_elf(const char *elf_file) {
  FILE* fp = fopen(elf_file, "rb");
  if(fp == NULL){
  	log_write("Elf_file isn't exit.\n");
	printf("Elf_file isn't exit.\n");
	return ;
  }
  // the size of elf_file
  fseek(fp, 0, SEEK_END);  // 将读写位置指向文件尾后，再增加 offset(0) 个偏移量为新的读写位置
  long flen = ftell(fp);   // 文件位置指针当前位置相对于文件首的偏移字节数
  rewind(fp); 		   //将文件内部的位置指针重新指向一个流（数据流或者文件）的起始位置
  
  // Judge the length of this file longer than the size of Ehdr struct
  assert(flen >= sizeof(Elf32_Ehdr));

  // read the ELF header
  Elf32_Ehdr Ehdr = read_system_tab(fp);

  // read the funcinfo from this EHdr;
  read_elf_func(Ehdr, fp);

  fclose(fp);
}
