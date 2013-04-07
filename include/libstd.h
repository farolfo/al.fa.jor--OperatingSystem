/**********************
 libstd.h
**********************/

#ifndef _libstd_
#define _libstd_

#define NULL 0

int printf(const char * , ...);
int scanf(const char * ,...);
int putchar(int);
int getchar();
int getCantDig(int);
int pow(int,int);
int putc(int,int);
int getc(int);
int stratoi(char * /*talveez*/);
int isDigit(int);
int bprintf(const char*);
void memcpy(char* a, char* b, int len);
int random();
#endif
