#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
 //  panic("Not implemented");
  size_t len = 0;
  while(s[len] != '\0') len++;
  return len;
}

char *strcpy(char *dst, const char *src) {
  // panic("Not implemented");
  size_t len = strlen(src);
  for(size_t i = 0; i < len;i++) dst[i]=src[i];
  dst[len] = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  // panic("Not implemented");
  size_t i;
  for(i =0; i < n && src[i]!='\0'; i++){
  	dst[i] = src[i];
  }
  for( ; i < n; i++) 
	dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  // panic("Not implemented");
  size_t dst_len = strlen(dst);
  size_t i;
  for (i = 0 ; i < dst_len ; i++)
	  dst[dst_len + i] = src[i];
  dst[dst_len + i] = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  // panic("Not implemented");
  size_t i = 0;
  for(; s1[i]!='\0' && s2[i]!='\0';i++){
  	if(s1[i] < s2[i]) return -1;
	else if(s1[i] > s2[i]) return 1;
  }
  if(s1[i] == s2[i]) return 0;
  else if(s1[i]=='\0') return -1;
  return 1;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  // panic("Not implemented");
  if(s1 == NULL && s2 == NULL) return 0;
  size_t i = 0;
  for(; i < n && s1[i]!='\0' && s2[i]!='\0';i++){
	 if(s1[i] < s2[i]) return -1;
	 else if(s1[i] > s2[i]) return 1;
  }
  if(i == n || s1[i] == s2[i]) return 0;
  else if(s1[i] =='\0') return -1;
  return 1;
}

void *memset(void *s, int c, size_t n) {
  // panic("Not implemented");
  if(s == NULL) return s;
  unsigned char* src = s;
  for(size_t i = 0; i < n; i++){
  	src[i] = c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  if(src == NULL|| dst ==NULL || n==0 || dst == src) return NULL;
  unsigned char *dest = dst;
  const unsigned char *source = src;
  if (dst < src) {
	  while (n != 0) {
		  --n;
		  *dest = *source;
		  ++dest;
		  ++source;
	  }
  } 
  else {
	  dest += n;
	  source += n;
	  while (n != 0) {
		  --n;
		  --dest;
		  --source;
		  *dest = *source;
	  }
  }
  return dst;  
}

void *memcpy(void *out, const void *in, size_t n) {
  // panic("Not implemented");
  if(out == NULL|| in ==NULL || n==0 || out == in) return NULL; 
  unsigned char *dest = out;
  const unsigned char *src = in;
  while(n != 0){
	*dest = *src;
	n--;
	dest++;
	src++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  // panic("Not implemented");
  if(s1 == NULL && s2 == NULL) return 0;
  const unsigned char *src1 = s1;
  const unsigned char *src2 = s2;
  size_t i = 0;
  for(; i < n && src1[i]!='\0' && src2[i]!='\0';i++){
	  if(src1[i] < src2[i]) return -1;
	  else if(src1[i] > src2[i]) return 1;
  }
  if(i == n || src1[i] == src2[i]) return 0;
  else if(src1[i] =='\0') return -1;
  return 1;
}

#endif
