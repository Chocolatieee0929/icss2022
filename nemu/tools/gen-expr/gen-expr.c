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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
int count = 0;
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
static int choose(int num){
   return rand()%num;
}
// rand_num
static unsigned gen_num(){
  int len = choose(32);
  for(int i=0;i<len;i++){
  	char ch = '0'+choose(10);
	gen(ch);
  }
  return 0;
}
// rand_op
static char gen_rand_op(){
  char op[4]={'+','-','*','/'};
  int r = choose(4);
  gen(op[r]);
  return 0;
}
// gen()
static void gen(char op){
  if(count == 65536){
  	printf("No empty.");
	return;
  }
  buf[count++] = op;
  return ;
}
// Space
static void space(){
  int remain = 65536 - count;
  remain = rand(remain);
  for(int i = 0;i<remain;i++) gen(' ');
  return;
}

static void gen_rand_expr() {
  //buf[0] = '\0';
  int i = choose(3);
  if(count > 20) i = 0;
  switch(i){
	case 0: 
	  gen_num(); 
	  break;
	case 1:
	  gen('('); gen_rand_expr(); gen(')'); 
	  break;
	default: 
	  gen_rand_expr(); gen_rand_op(); gen_rand_expr(); 
	  break;
  }

}

int main(int argc, char *argv[]) {
  // combine time and random seed
  int seed = time(0);
  srand(seed);
  // rand_expr nums
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr();
    // code_buf = code_format计算器 + buf
    sprintf(code_buf, code_format, buf);
    
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    //execute a shell command
    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    // popen, pclose - pipe stream to or from a process
    // FILE *popen(const char *command, const char *type);
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
