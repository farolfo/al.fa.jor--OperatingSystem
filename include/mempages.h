#ifndef _mempages_
#define _mempages_

typedef struct Header {
	int status;
	struct Header * next;
	int size;
} Header;

void * malloc(int size);

void free(void * memDir);

void setPages();

void * calloc(int cant, int size);

void listMemory();

unsigned int malloc_user(char *,int);
void free_user(char*);
unsigned int calloc_user(char *,int);

#endif
