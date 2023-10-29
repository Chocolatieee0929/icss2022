#ifdef CONFIG_FTRACE
#include <elf.h>
#include <common.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define FUNCNAME_SIZE 32 
#define Func_num 128 

typedef struct{
  char funcname[FUNCNAME_SIZE];
  paddr_t func_start;
  size_t func_size;  
}FuncInfo;

FuncInfo Elf_func[Func_num];
void init_elf(const char *elf_file);

#endif
