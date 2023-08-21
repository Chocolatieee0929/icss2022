#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

int count = 0;
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { \n"
"  unsigned result = %s; \n"
"  printf(\"%%u\", result); \n"
"  return 0; \n"
"}\n";

static int choose(int num){
    return rand() % num;
}

static void gen(char op) {
  sprintf(buf+count,"%c",op);
  count++;  
}

static void gen_num() {
   // Avoid the condition that result integer too long
   unsigned num = choose(100);
   sprintf(buf+count,"%u",num);
   while(buf[count]!=0)  count++;
   return;
}

static char gen_rand_op() {
    char op[4] = {'+', '-', '*', '/'};
    int r = choose(4);
    gen(op[r]);
    return 0;
}

/*static void space() {
    int remain = rand() % 10;
    for (int i = 0; i < remain; i++) gen(' ');
    return;
}*/

static void gen_rand_expr() {
    int i = choose(3);
    if (count > 20) i = 0;
    switch (i) {
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
    for (i = 0; i < loop; i++) {
        count = 0;
        gen_rand_expr();
        buf[count] = '\0';

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

        unsigned result;
        ret = fscanf(fp, "%d", &result);
        pclose(fp);

        printf("%u %s\n", result, buf);
    }

    return 0;
}
