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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <monitor/sdb.h>
#include <monitor/watchpoint.h>
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

//printf watchpoint information
/*
static void wp_print(){
   WP* h = head;
   if (!h) {
	   puts("No watchpoints.");
	   return;
   }
   printf("%-8s%-8s\n", "Num", "What");
   while (h) {
	   printf("%-8d%-8s\n", h->NO, h->expr);
	   h = h->next;
   }
}
*/
static int add_watchpoint(char* EXPR,word_t re){
  //printf("add_wp.\n");
  WP* p = new_wp();
  if(!p){
	puts("Please delete some points that you should not need if you want to add new.\n");
	return 0;
  }
  printf("new_wp.\n");
  p->new_val = re;
  // point would be changed if you add new EXPR
  // p->expr = EXPR;
  p->expr =(char*)malloc(strlen(EXPR)*sizeof(char));
  strcpy(p->expr,EXPR);
  printf("%-8s%-8s\n","Number","EXPR");
  printf("%-8d%-8s\n", p->NO, p->expr);
  // debug
  //wp_print();
  printf("Success to add!\n");
  return 0;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_si(char *args) {
  // 提取命令
  char *arg = strtok(NULL," ");
  // 单步执行的次数
  size_t step = 0;
  // 未传入参数则默认单步执行1
  if(arg == NULL){
	step = 1;
  }
  else{
	sscanf(arg,"%ld",&step);
	if(step<=0){
	   printf("Unknown arguments '%s'\n", arg);
	   return 0;
	}
  } 
  cpu_exec(step);
  return 0;
}

static int cmd_info(char *args){
   // 提取命令
  char *arg = strtok(NULL," ");
  if(!strcmp(arg, "r")){
      isa_reg_display();
  }
  else if(!strcmp(arg, "w")){
      wp_print();   
  }
  else {
	  printf("Usage: info r (registers) or info w (watchpoints)\n");
  }
  return 0; 
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_x(char *args){
  char *N =  strtok(NULL," ");
  char *EXPR = strtok(NULL," ");
  size_t len = 0;
  vaddr_t addr;
  // [N]
  if(N == NULL|| EXPR == NULL){
  	printf("Invalid parament.\n");
	return 0;
  }
  sscanf(N,"%ld",&len);
  sscanf(EXPR,"%x",&addr);
  //if(!(addr >= 0x80000000 && addr <=0x87ffffff))
  for(int i = 0; i<len; i++){
	if(!(addr >= 0x80000000 && addr <=0x87ffffff)){
		printf("addr should be in [0x80000000,0x87ffffff].\n");
		return 0;
	}
	printf("%x:%x\n",addr,vaddr_read(addr,4));
	addr += 4;
  }
  return 0;
}

static int cmd_p(char *args){
  char *EXPR = strtok(NULL,"");
  if(EXPR == NULL){
  	printf("Invaild parament.");
	return 0;
  }
  bool result = true ;
  bool hex = false;
  word_t re = expr(EXPR,&result,&hex);
  // debug
  if(!result){
  	printf("The EXPR is invalid, please try again.\n");
  }
  else if(hex) printf("result:%x\n",(sword_t)re);
  else printf("result:%d\n",(sword_t)re);
  return 0;
}
static int cmd_w(char *args){
  char *EXPR = strtok(NULL,"");
  if(EXPR == NULL){
  	printf("Invaild parament.");
	return 0;
  }
  bool result = true ;
  bool hex = false;
  word_t re = expr(EXPR,&result,&hex);
  // debug
  if(!result){
  	printf("failed\n");
  }
  else add_watchpoint(EXPR,re);
  return 0;
}

static int cmd_del(char *args){
  char *EXPR = strtok(NULL,"");
  if(EXPR == NULL){
  	printf("Invaild parament.");
	return 0;
  }

  word_t num = strtol(args, NULL, 10);
  // debug
  if(!num){
  	printf("Usage: d N (delete watchpoint_num,N>0).\n");
  }
  else{
	 free_wp(num-1);
	 wp_print();
  }
  return 0;
}

/*
static int cmd_ptest(char *args){
   FILE *f = fopen("/home/ics2022/nemu/tools/gen-expr/input","r");
   if(f==NULL){
	perror("The file dont exist.\n");
	return 0;
   }
   size_t sz=0;
   char *line = NULL;
   while(getline(&line,&sz,f)>0){
   	word_t correct = atoi(strtok(line,""));
   	char *EXPR = strtok(NULL,"");
  	 bool result = true ;
   	word_t re = expr(EXPR,&result);
      	if(!result || re != correct){
		printf("failed\n");
   	}
   	printf("result:%d\n",re);
   }
   return 0;
}
*/

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"si","step-让程序单步执行N条指令后暂停执行,当N没有给出时,默认为1, eg: si [N]",cmd_si},
  {"info","打印寄存器状态/监视点信息.eg: info SUBCMD",cmd_info},
  {"x","求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节, eg:x [N] EXPR",cmd_x},
  {"p","求出表达式EXPR的值 p $eax + 1",cmd_p},
 // {"p_test","cmd_p test ,file in ./input",cmd_ptest},
 {"w","当表达式EXPR的值发生变化时, 暂停程序执行 w *0x2000",cmd_w},
  {"d","删除序号为N的监视点 d 2",cmd_del}, 
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void test_expr(){
   FILE *f = fopen("./tools/gen-expr/input","r");
   if(f==NULL){
	perror("The file dont exist.\n");
	return ;
   }
   size_t sz=0;
   char *line = NULL;
   ssize_t read;
   word_t res;
   word_t correct_res;

   while(getline(&line,&sz,f)>0){
	//printf("第%lu个测试案例：%s\n",sz,line);
	bool success = true;
	read = getline(&line,&sz,f);
	line[read-1]='\0';
       char nline[strlen(line)];

       strcpy(nline,line);
       char* num = strtok(nline," ");
       correct_res = atoi(num);
       num = strtok(NULL,"");
       bool hex = false;
       res = expr(num,&success,&hex);

       assert(success);
       if(res != correct_res){
	  printf("expected: %d, got: %d\n", correct_res, res);
	  assert(0);
       }
   }
   printf("All expr_tests are passed !\n");
   /*
   while (true) {
	   bool success = true;
	   if(fscanf(f, "%u ", &correct_res) == -1) break;
   	   read = getline(&line, &sz, f);
	   puts(line);
	   line[read-1] = '\0';

	   res = expr(line, &success);
			       
	   assert(success);
	   if (res != correct_res) {
		   puts(line);
		   printf("expected: %u, got: %u\n", correct_res, res);
		   assert(0);
	   }
   }
   */
   fclose(f);
   free(line);
   return ;

}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* test regex*/
 //test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
