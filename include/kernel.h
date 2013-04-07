
/********************************** 
*
*  Kernel
*
***********************************/

#ifndef _kernel_
#define _kernel_
#include "../include/defs.h"

#define OS_PID	0


int (*player)(void);

typedef int size_t;
typedef short int ssize_t;
//typedef enum eINT_80 {WRITE=0, READ} tINT_80;
typedef enum eUSER {U_KERNEL=0, U_NORMAL} tUSERS;

/* __write
*
* Recibe como parametros:
* - File Descriptor
* - Buffer del source
* - Cantidad
*
**/
size_t __write(int fd, const void* buffer, size_t count);


/* __read
*
* Recibe como parametros:
* - File Descriptor
* - Buffer a donde escribir
* - Cantidad
*
**/
size_t __read(int fd, void* buffer, size_t count);

void _cursor(int);

void 	int_80(int, int, void *, int);

void	backspace(int);

void destroy(PROCESS * p);

void * kmalloc(int size);

void exec(char* name, int (*process)(int,char**), int argc, char** argv, int priority, int isFront, int maxHeapPages, int maxStackPages);


void clear();

void waiti(int time);

void yield();

void printToScreen(int, char *);

int useSector(int, char *, int, int);
#endif
