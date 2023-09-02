/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          h/ttp://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* S/ee t* She Mulan PSL v2 for more details.
***************************************************************************************/

//#include <common.h>
#include <monitor/watchpoint.h>
#include <monitor/sdb.h>
//#define NR_WP 32

/*
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  // TODO: Add more members if necessary 
  char *expr;
  uint32_t pre_val;
  uint32_t new_val;
} WP;
*/

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].pre_val=0;
    wp_pool[i].new_val=0;
    wp_pool[i].expr=NULL;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

bool is_wps_diff(){
  WP* tmp = head;
  wp_print();
  int flag = 0;
  if(!tmp){
  	return false;
  }
  while(tmp){
	// debug
	printf("Now NO.%d:%s\t",tmp->NO,tmp->expr);
  	tmp->pre_val = tmp->new_val;
	bool result = true ;                                                                        
	bool hex = false;                                                                           
	tmp->new_val = expr(tmp->expr,&result,&hex);
	assert(result);
	if(tmp->new_val!=tmp->pre_val){
		if(!flag) printf("%s\t%s\t%s\t%s\t\n","Number","EXPR","Pre_val","New_val");
		if(hex) printf("%d\t%s\t%x\t%x\t\n\n", tmp->NO, tmp->expr, tmp->pre_val, tmp->new_val);
		else printf("%-8d%-8s%-16d%-16d\n\n", tmp->NO, tmp->expr, tmp->pre_val, tmp->new_val);
		flag = 1;
	}
	tmp = tmp->next;
  }
  if(flag) return true;
  return false;
}

void wp_print(){
   WP* h = head;
   if (!h) {
           puts("No watchpoints.\n");
           return;
   }
   printf("%-8s%-8s\n", "Num", "EXPR");
   while (h) {
           printf("%-8d%-8s\n", h->NO, h->expr);
           h = h->next;
   }
}

WP* new_wp(){
   if(free_==NULL){
   	printf("No empty.\n");
	//assert(0);
	return NULL;
   }
   WP* tmp = free_;
   free_ = free_->next;
   tmp->next = head;
   head = tmp;
   return tmp;
}

void free_point(WP* wp){
   assert(wp);
   if(wp==head){
	head = head->next;
   }
   else{
   	WP *pre = head;
   	while(pre && pre->next != wp) pre=pre->next;
	assert(pre);
   	pre->next = wp->next;
   }
   wp->next = free_;
   free_ = wp;

}

int free_wp(int num){
   if(num < 0 || num > head->NO){
        printf("N should be in [1,%d]\n",head->NO+1);
	return -1;
  }
     WP* tmp = head;
     while(tmp && tmp->NO > num){
	     tmp->NO--;
	     tmp = tmp->next;
     }
     tmp->NO = free_->NO;
     free_point(tmp);
     printf("Success to delete this watchpoint.\n");
     return 0;
}

