#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  //panic("Not implemented");
  va_list args;
  int n;
  char s[128];
  // putch('Y');
  va_start(args,fmt);
  n = vsprintf(s,fmt,args);
  va_end(args);
  return n;
}

int numlen(int num){
  int len = 0;
  for(;num>0;num/=10) len++;
  return len; 
}

void str_num(int n, char* buff){
  int len = numlen(n);
  char s[len];
  for(int i = len-1;i>=0;i--){
    int temp = n%10;
    s[i]='0'+temp;
    n/=10;
  }
  strcpy(buff,s);
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  // panic("Not implemented");
  char *buff = out;
  int len = 0;
  //va_start(ap, fmt);
  for(; *fmt!='\0'; fmt++){
      if(*fmt != '%'){
	  *buff = *fmt;
	   buff ++;
	   len++;
	   // debug
	   // putch('Y');
      }
      else{
	fmt ++;
	switch(*fmt){
	  case 's':
		char *s = va_arg(ap, char *);
		if(s==NULL) s="<NULL>";
		strcpy(buff, s);
		// putch(*buff);
		// putch('\n');
		printf(buff);
		len += strlen(s);
		buff += strlen(s); 
		break;
	  case 'd':
		int d = va_arg(ap, int);
		int dlen = numlen(d); 
		str_num(d,buff);
		//strcpy(buff,num);
		len += dlen;
		//putch(*buff);
		buff += dlen;
		break;
	  case 'c':
		char c = va_arg(ap, int);
		*buff = c;
		len ++;
		buff ++;
		break;
	} // switch
      } // else
  } // for
  *buff = '\0';
  //va_end(ap);
  return len;
}

int sprintf(char *out, const char *fmt, ...) {
  // panic("Not implemented");
  va_list ap;
  //char *buff = out;
  int n = 0;
  va_start(ap, fmt);
  n = vsprintf(out,fmt,ap);
  //printf("n:%d",n);
  va_end(ap);
  return n;
  /*
  for(; *fmt!='\0'; fmt++){
      if(*fmt != '%'){
	  *buff = *fmt;
	   buff ++;
	   len++;
      }
      else{
	fmt ++;
	switch(*fmt){
	  case 's':
		char *s = va_arg(ap, char *);
		if(s==NULL) s="<NULL>";
		strcmp(buff, s);
		len += strlen(s);
		buff ++; 
		break;
	  case 'd':
		int d = va_arg(ap, int);
		len += str_num(d, buff); 
		buff ++;
		break;
	  case 'c':
		char c = va_arg(ap, int);
		*buff = c;
		len ++;
		buff ++;
		break;
	} // switch
      } // else
  } // for
  *buff = '\0';
  va_end(ap);
  return len;
  */
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
