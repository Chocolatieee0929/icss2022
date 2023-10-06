#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
  //va_list ap;
  //int d;
  //char c;
  //char *s;
  //va_start(ap,word);
  // const char fmt
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int str_num(int n, char *s){
  int len = 0;
  int num = n;
  for(;num>0;num/=10) len++;
  for(int i=len ; i>0; i--) {
	s[i-1] = n%10;
	n /= 10;
  }
  return len; 
}

int sprintf(char *out, const char *fmt, ...) {
  // panic("Not implemented");
  va_list ap;
  char *buff = out;
  int len = 0;
  va_start(ap, fmt);
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
		strcmp(buff, s);
		len += strlen(s);
		buff ++; 
		break;
	  case 'd':
		int d = va_arg(ap, int);
		len += str_num(d, buff); 
		buff ++;
		break;
	} // switch
      } // else
  } // for
  *buff = '\0';
  va_end(ap);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
