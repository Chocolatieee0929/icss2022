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
#include <elf.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <common.h>

#define FUNCNAME_SIZE 32 
#define Func_num 128 

typedef struct{
	  char func_name[FUNCNAME_SIZE];
	  Elf32_Addr func_start;
	  size_t func_size;
}FuncInfo;

FuncInfo Func[Func_num];
void init_elf(const char *elf_file);
FILE *elf_fp = NULL;

//=================================== func_trace ==========================================//
typedef struct func_call{
  FuncInfo to_func;
  struct func_call *next; 
}fentry;

int fentry_num = 0;
fentry *fstart = NULL;
fentry *fend = NULL; 

int addrtofunc(paddr_t addr){
  for(int i = 0;Func[i].func_name[0]!=0;i++){
	size_t start = Func[i].func_start;
	size_t size = Func[i].func_size;
	if(addr >= start && addr < start + size)
	  return i;
  }
  return -1;
}

void func_trace(paddr_t pc,paddr_t target){
  // 找到from地址
  // int from = addrtofunc(pc);
  printf("0x%x:  ",pc); 
  // 函数回退	
  if(target==0){
	if(!fentry_num||fstart->next){
	  printf("没有调用过函数。\n");
	  return;
	}
	// 1.从开头退出函数，并打印
	fentry* temp = fstart->next;
	FuncInfo func = temp->to_func;
	fstart->next = temp->next;
	for(int i=0;i<fentry_num;i++) printf(" ");
	printf("call [%s]\n",func.func_name);
	// 2.函数条目数目减少
	free(temp);
	fentry_num--;
	return;
  }
  // 函数调用
  // 1.找到目标函数
  int to = addrtofunc(target);
  printf("0x%x:   ",pc);
  // 2.trm_init和main函数
  if(strcmp(Func[to].func_name,"_trm_init")!=0 && strcmp(Func[to].func_name,"main")!=0){
	for(int i=0;i<fentry_num;i++) printf(" "); 
	fentry *temp = malloc(sizeof(fentry));
 	temp->to_func = Func[to];
	temp->next = NULL;
  	fend->next=temp; fend = temp; 
	fentry_num++;
  }
  printf("call [%s@0x%x]\n",Func[to].func_name, Func[to].func_start);  
  return;
}

//=============================== init func_table =========================================//
static Elf32_Ehdr read_system_tab(FILE *fp){
  char headbuf[EI_NIDENT] = {0};
  rewind(fp);
  assert(fread(headbuf, sizeof(char), EI_NIDENT, fp));
  if(headbuf[0] !=0x7f && headbuf[1] != 'E' 
		  && headbuf[2] != 'L' && headbuf[3] != 'F')
	assert(0);
  rewind(fp);
  Elf32_Ehdr elfHeader32;
  assert(fread(&elfHeader32, sizeof(Elf32_Ehdr), 1, fp));
  rewind(fp);
  return elfHeader32;
}

int read_elf_symtab(Elf32_Sym *elf_symtab, Elf32_Shdr sec_sym, FILE *fp) {
  // 读取章节表的对应是函数的条目
  int symnum = sec_sym.sh_size / sec_sym.sh_entsize;
  // debug
  // printf("sh_size:%x sh_entsize:%x  ", sec_sym.sh_size, sec_sym.sh_entsize);
  // printf("symnum: %d\n",symnum);
  if((sec_sym.sh_size % sec_sym.sh_entsize) != 0){ 
	printf("没有整数个数目\n");
	return 0;
  }
  // Elf32_Sym elf_func[symnum];
  rewind(fp);
  fseek(fp, sec_sym.sh_offset, SEEK_SET);
  
  int funcnum = 0;
  for(int i = 0; i < symnum; i++){
	Elf32_Sym symentry ;
	assert(fread(&symentry, sizeof(Elf32_Sym),1,fp));
	// debug
	// printf("symname: %x tfunc_start = 0x%x\t func_offset = 0x%x\n", 
	// 		symentry.st_name, symentry.st_value, symentry.st_size);
	if(ELF32_ST_TYPE(symentry.st_info) == STT_FUNC){
		elf_symtab[funcnum] = symentry;	
		// debug
		// printf("func:%d",funcnum);
		funcnum++;
	}
  }
  /*
  if(funcnum != 0){
	//elf_symtab = (Elf32_Sym*)malloc(funcnum * sizeof(Elf32_Sym));
	for(int i = 0; i < funcnum; i++){
	   elf_symtab[i] = elf_func[i];	
	}
  }
  */
  rewind(fp);
  return funcnum;
}

// 从符号表中type为FUNC类型将其转换成我们定义的结构体，函数名需要根据字符串表进行读取
int Convert_FuncInfo(Elf32_Sym *elf_symtab, int symnum, Elf32_Shdr elfstrtab,FILE * fp){
	size_t index = 0;
	// Elf32_Sym * st = elf_symtab;
	for(; index < symnum; index++){
		// dedbug
		// printf("now:%ld ",index);
		Elf32_Sym symentry = elf_symtab[index];
		size_t offset = elfstrtab.sh_offset + symentry.st_name;
		// debug
		//printf("symname: %x \tfunc_start = 0x%x\t func_offset = 0x%x\n",
		//	symentry.st_name, symentry.st_value, symentry.st_size);
		//printf("offset: %lx\n",offset);
		
		fseek(fp, offset, SEEK_SET);
		assert(fscanf(fp, "%s", Func[index].func_name) > 0);
		// debug
		// printf("funcname: %s\n",Func[index].func_name);
		Func[index].func_start = symentry.st_value;
		// debug
		// printf("func_start = 0x%x\n",Func[index].func_start);
		Func[index].func_size  = symentry.st_size;
		// debug
		// printf("func_size = 0x%lx\n",Func[index].func_size); 
		// debug
		printf("funcname: %s\t func_start = 0x%x\t func_offset = 0x%lx\n", 
				Func[index].func_name, Func[index].func_start,
				Func[index].func_size);
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
  Elf32_Sym elf_symtab[50];
  size_t symnum = 0;
  Elf32_Shdr elf_strtab;
  
  // 从Section Header里读出 section name strtab
  fseek(fp, shoff + Ehdr.e_shstrndx * Ehdr.e_shentsize, SEEK_SET);
  assert(fread(&elf_shstrtab, sizeof(Elf32_Shdr), 1, fp));
  // debug 
  // printf("nameoffset: %x\n",elf_shstrtab.sh_offset);
  rewind(fp);

  // 从Section Header里读出字符串表和符号表
  // fseek(fp, shoff, SEEK_SET);
  // int flag = 1;
  for(size_t i = 0; i < shnum; i++){
	fseek(fp, shoff+ i* sizeof(Elf32_Shdr),SEEK_SET);
	Elf32_Shdr secEnt;
	assert(fread(&secEnt, sizeof(Elf32_Shdr), 1, fp));
	if(secEnt.sh_type == SHT_SYMTAB){
		symnum = read_elf_symtab(elf_symtab, secEnt, fp);
		// debug
		//printf("test:%x\n",elf_symtab[0].st_value);
		//printf("symnum:%lx\n",symnum);
		assert(symnum);
	}
	if(secEnt.sh_type == SHT_STRTAB && i!=Ehdr.e_shstrndx){
		elf_strtab = secEnt;
		// flag = 0;
		// printf("nameoffset: %x\n",elf_strtab.sh_offset); 
	}
  }
  
  // 从符号表中type为FUNC类型将其转换成我们定义的结构体，函数名需要根据字符串表进行读取
  assert(Convert_FuncInfo(elf_symtab, symnum, elf_strtab, fp)==0);
  // free(elf_symtab);
  // rewind(fp);
  // fclose(fp);
}

// 初始化形成elf文件
void init_elf(const char *elf_file) {
  fentry_num = 0;
  FILE* fp = fopen(elf_file, "rb");
  if(fp == NULL){
  	log_write("Elf_file isn't exit.\n");
	printf("Elf_file isn't exit.\n");
	return ;
  }
  // initialize the func_entry
  fstart = malloc(sizeof(fentry));
  fstart->next = NULL;
  fend = fstart;

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
  
  rewind(fp);
  fclose(fp);
}

