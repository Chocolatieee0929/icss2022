#pragma once

#include "monitor/sdb.h"

#define NR_WP 32
#define NR_WP_EXPR_MAX 3000

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */	
  	char *expr;
	uint32_t pre_val;
	uint32_t new_val;
} WP;

//static WP wp_pool[NR_WP] = {};
//static WP *head = NULL, *free_ = NULL;

void init_wp_pool();
WP* new_wp();
int free_wp(int num);
void wp_print();
bool is_wps_diff();
bool is_exit_wp(char* EXPR);
