/***************************************************
  Defs.h
	
****************************************************/

#ifndef _defs_
#define _defs_

#include "circularBuffer.h"
#include "filesystem.h"

/*Definicones de Semaforo*/
#define	SEMAPHORE	int
#define	UP			1
#define	DOWN		0

#define byte unsigned char
#define word short int
#define dword int

/* Flags para derechos de acceso de los segmentos */
#define ACS_PRESENT     0x80            /* segmento presente en memoria */
#define ACS_CSEG        0x18            /* segmento de codigo */
#define ACS_DSEG        0x10            /* segmento de datos */
#define ACS_READ        0x02            /* segmento de lectura */
#define ACS_WRITE       0x02            /* segmento de escritura */
#define ACS_IDT         ACS_DSEG
#define ACS_INT_386 	0x0E		/* Interrupt GATE 32 bits */
#define ACS_INT         ( ACS_PRESENT | ACS_INT_386 )


#define ACS_CODE        (ACS_PRESENT | ACS_CSEG | ACS_READ)
#define ACS_DATA        (ACS_PRESENT | ACS_DSEG | ACS_WRITE)
#define ACS_STACK       (ACS_PRESENT | ACS_DSEG | ACS_WRITE)

#define NULL		0

#define SCREEN		0
#define SPEAKER		1

#define KEYBOARD	0

#define WRITE		0
#define READ		1
#define TICKPOS		2
#define CURSOR		3
#define BACKSPACE	4
#define PRINTSOMEWHERE	5
#define MAX_PROCESS_NAME	30
#pragma pack (1) 		/* Alinear las siguiente estructuras a 1 byte */

/* Descriptor de segmento */
typedef struct {
  word limit,
       base_l;
  byte base_m,
       access,
       attribs,
       base_h;
} DESCR_SEG;


/* Descriptor de interrupcion */
typedef struct {
  word      offset_l,
            selector;
  byte      cero,
            access;
  word	    offset_h;
} DESCR_INT;

/* IDTR  */
typedef struct {
  word  limit;
  dword base;
} IDTR;

enum status { RUNNING = 0, BLOCKED, READY};
enum blocked { NOT_BLOCKED = -1, BLOCK_READ  = 0, BLOCK_WAITCHILD = 1, BLOCK_WAIT = 2};


typedef struct TTY
{
	void * outputBuffer;
	circularBuffer inputBuffer;
	int tickpos;
	int format;
	ENTRY cwd;
	ENTRY pcwd;
} TTY;

typedef struct PROCESS
{
	int pid;
	char name [MAX_PROCESS_NAME];
	int priority;
	TTY * tty;
	int foreground;
	int lastCalled;
	int status;
	int blocked;
	int parentPid;
	int ESP;
	int stackstart;
	int stacksize;
	void ** heapPages;
	void * firstHeapPage;
	void * lastHeapPage;
	int maxHeapPages;
	int maxStackPages;
	int ticks;
	int waitingTime;
	int cursor;
} PROCESS;



typedef struct
{
	int EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX,  EIP, CS, EFLAGS;
	void*retaddr;
	int argc;
	char** argv;
} STACK_FRAME; 

#endif

