/**********************
 string.h
**********************/

#ifndef _string_
#define _string_

void strspcat(const char *, char*, char*);
int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char toupper(char s);
char tolower(char s);
void strcpy(char *, char*);
int endsWith(char *,char);
void charcat(char *, char);
char ** split(char *, char, int *);
void strcat(char *, char *);

#endif
