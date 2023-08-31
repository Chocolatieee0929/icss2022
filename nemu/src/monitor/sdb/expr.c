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
*/
#include <isa.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, 

  TK_DEX, TK_HEX,TK_NEG,
  TK_RV,TK_EQ, TK_GT, TK_LT, TK_GE, TK_LE, 
  TK_AND,TK_OR,  TK_REG,TK_DEREF, 
  //TK_VAR,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    // spaces

  {"==", TK_EQ},        // equal
  {"\\!=", TK_RV},	// reverse
  {"<=", TK_LE}, {">=", TK_GE}, {"<", TK_LT}, {">", TK_GT}, 
  {"&&", TK_AND},	// and
  {"\\|\\|", TK_OR}, 	// or
			
  {"\\+", '+'},         // plus
  {"\\-", '-'},		// substract
  {"\\*", '*'},		// multiply
  {"\\/", '/'},		// chuyi

  {"\\(",'('},		// bracket'('
  {"\\)",')'},		// bracket')'

  {"0x[0-9]+",TK_HEX}, 	// hex_num
  {"[0-9]+", TK_DEX},	// dex_number

  {"\\$\\w+",TK_REG},	// reg_name
  //{"[A-Za-z_]\\w*",TK_VAR},    // 指针引用
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

// token array
static Token tokens[64] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
	  case TK_NOTYPE:
		  break;
	  case TK_HEX:
		  tokens[nr_token].type=TK_HEX;
		  for(int i=0;i<substr_len&&32;i++){
			tokens[nr_token].str[i] = substr_start[i];
		  }
		  tokens[nr_token].str[substr_len]='\0';
		  nr_token++;
		  break;
	  case TK_DEX:
		  tokens[nr_token].type=TK_DEX;
		  for(int i=0;i<substr_len&&32;i++){
			tokens[nr_token].str[i] = substr_start[i];
		  }
		  tokens[nr_token].str[substr_len]='\0';
		  nr_token++;
		  break;
	  case TK_EQ: 
		  tokens[nr_token].type = TK_EQ;
		  nr_token++;
		  break;
	  case TK_RV:
		  tokens[nr_token].type = TK_RV;
		  nr_token++;
		  break;
	  case TK_AND:
		  tokens[nr_token].type = TK_AND;
		  nr_token++;
		  break;
	  case TK_OR: 
		  tokens[nr_token].type = TK_OR;
		  nr_token++;
		  break;
	  case TK_REG:
		  tokens[nr_token].type = TK_REG;
		  for(int i=0;i<substr_len&&32;i++){
			tokens[nr_token].str[i] = substr_start[i];
		  }
		  tokens[nr_token].str[substr_len]='\0';
		  nr_token++;
		  break;
	  /*case '*' :
		  if(nr_token==0||tokens[nr_token-1].type=='('){
			printf("指针解引用.\n");
		  	tokens[nr_token].type = TK_DEREF;
			for(int i=0;i<substr_len&&32;i++){                                  
				tokens[nr_token].str[i] = substr_start[i];                            
				tokens[nr_token].str[substr_len]='\0';
    			}	
			nr_token++;
		   }	
		  else tokens[nr_token++].type = '*';
		  break;
	  */
          default:
		  tokens[nr_token++].type = rules[i].token_type; 
		  break;
	}
      break;
      }
    }

    if (i == NR_REGEX) {
     /*识别失败, 框架代码会输出当前token的位置
      * (当表达式过长导致在终端里输出需要换行时, ^可能无法指示正确的位置, 
      * 此时建议通过输出的position值来定位token的位置
      */
      // printf("Position: %d\n", position);
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }
  return true;
}

// Judge legal braket 
bool check_parentheses(int p,int q){
   if(tokens[p].type != '('  || tokens[q].type != ')')
        return false;
   //int l=p;int r=q;
   int num=0;

   for(int i=p;i<=q;i++){
	if(tokens[i].type == '(') num++;
	if(tokens[i].type == ')') num--;
	if(num <= 0 && i!=q)       return false;
   }
   if(num != 0) return false;
   /*
   while(l<r){
	if(tokens[l].type=='('){
	  if(tokens[r].type == ')'){
	  	l ++ , r --;continue;
	  }
	  else r--;
	}
	else if(tokens[l].type == ')'){
		// debug
	       	printf("l:%d",l);
		return false;
	}
	else l++;
   }
   */
   return true;
}

// 寻找主运算符
int mainToken(int p,int q){
    int mainindex = p;
    int flag = 0;
    for(int i = p; i<q;i++){
     if(tokens[i].type==TK_AND||tokens[i].type==TK_OR){
	// debug
	printf("&& ||\n");
      	mainindex = i;
	flag = 3;
      }// != ==
      if(flag<3 && (tokens[i].type==TK_EQ||tokens[i].type==TK_RV)){
	// debug
	printf("!= ==\n");
      	mainindex = i;
	flag = 2;
      } 
      if(flag<2 && (tokens[i].type == '+'||tokens[i].type=='-')){
	mainindex = i;
	flag = 1;
      }
      else if(flag < 1 && (tokens[i].type == '*'||tokens[i].type =='/')){
	mainindex=i;
	flag = 0;
      }
      if(tokens[i].type == '('){
      	int tail = q;
	while(tail > i && check_parentheses(i,tail)!=true) tail--;
        if(tail==i) return -1;
	i = tail;       
      }
    }
    return mainindex;
}

// evaluate the val of expr
word_t eval(int begin,int end, bool *success){
  //debug
  word_t val = 0;
  printf("begin:%d, end:%d\n",begin,end);
  if(begin > end || *success == false){
  /* Bad expression */
      printf("Invalid expression.\n");
      *success = false;
      return 0;
  }
  else if(begin == end){
      if(tokens[begin].type==TK_REG){
	char* str =strtok(NULL,"$");  
	val = isa_reg_str2val(str, success);
      }
      else if(tokens[begin].type!=TK_DEX && tokens[begin].type!=TK_HEX) {
	  printf("Error exp, not number.\n");
	  *success = false;
	  return 0;
      }
      else if(tokens[begin].type==TK_HEX){
      	val = strtol(tokens[begin].str, NULL, 16);
      }
      else val = strtol(tokens[begin].str, NULL, 10);
      printf("str:%s\n",tokens[begin].str);
      printf("num:%u\n",val);
  }
  else if (check_parentheses(begin, end) == true){
  	val= eval(begin+1,end-1,success);
  }
  // else if((tokens[begin].type == '-' && begin == 0)
  //		  ||(tokens[begin].type == '-' && begin > 0 && tokens[begin-1].type != TK_DEX && tokens[begin+1].type == TK_DEX)){
  else if(tokens[begin].type == TK_NEG && tokens[end].type == TK_DEX && begin+1==end){
      unsigned val1 = eval(begin+1, end,success);
       val =val1*(-1);
  
  }
  else{
	int op = mainToken(begin,end);
	//debug
	printf("op:%d,%c\n",op,tokens[op].type);
	word_t val1 = eval(begin, op - 1,success);
	word_t val2 = eval(op + 1, end,success);
	switch (tokens[op].type) {
	      case TK_EQ:
		      val = val1==val2;
		      break;
	      case TK_RV:
		      val = val1!=val2;
		      break;
	      case TK_GT: 
		       val = val1 > val2;
		       break;
	      case TK_LT: 
		       val = val1 < val2;
		       break;
	      case TK_GE:  
		       val = val1 >= val2;
		       break;
	      case TK_LE: 
		       val = val1 <= val2;
		       break;
	      case TK_AND:
		      val = val1&&val2;
		      break;
	      case '+': 
		      val=val1 + val2;
		      break;
	      case '-':
		     val =  val1 - val2;		
		     break;
	      case '*':
		     val = val1 * val2;
		     break;
	      case '/': 
		     int flag = 0;
		     //debug
		     // printf("%d/%d",val1,val2);
		     if(val2==0){
	    		     printf("Invalid Expression.\n");
    			     *success = false;
			     return 0;
		     }
		     else if(val1<0){
		     	val1 *= -1;
			printf("val1:%d",val1);
			flag = 1;
		     }
		     else if(val2<0){
			val2 *= -1;
			if(flag) flag = 0;
			else flag = 1;
		     }
		     val = (sword_t)val1 / (sword_t)val2;
	     	     // debug
		     // printf("=%d\n",val);
     		     break;
	      default: assert(0);
	}
  }
  // printf("val:%d\n",val);
  return val;
}

word_t expr(char *e, bool *success) {
  // debug
  puts(e);
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i ++) {	 
	if (tokens[i].type == '*' &&  (i == 0|| tokens[i-1].type == '('
			       || tokens[i-1].type == TK_OR || tokens[i-1].type == TK_AND
			       || tokens[i-1].type == TK_EQ || tokens[i-1].type == TK_RV)) {
    		tokens[i].type = TK_DEREF;
	}
	if (tokens[i].type == '-' && (i == 0|| tokens[i-1].type == '('
			       || tokens[i-1].type == TK_OR || tokens[i-1].type == TK_AND
			       || tokens[i-1].type == TK_EQ || tokens[i-1].type == TK_RV) ) {
		tokens[i].type = TK_NEG;
		printf("------\n");
	}
  }

  *success = true;

  word_t result = eval(0,nr_token-1,success);
  if(*success != true)  result = 0;
  return result;
}
