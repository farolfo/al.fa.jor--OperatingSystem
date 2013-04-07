#include "../include/kasm.h"
#include "../include/defs.h"
#include "../include/libstd.h"
#include "../include/circularBuffer.h"
#include "../include/kc.h"
#include "../include/kernel.h"
#include "../include/speaker.h"
#include "../include/mempages.h"
#include "../include/shell.h"
#include "../include/sched.h"
#include "../include/string.h"
#include "../include/colormgmt.h"
#include "../include/filesystem.h"

#define DEBUG printf("lalal"); while(1);
#define P_DIR(dir, i, j) ( ( (unsigned int *) ( ( unsigned int ) ( ( ( unsigned int * ) pageDirectory )[i] ) & 0xfffff000 ) )[j] )

DESCR_INT idt[0x100];			/* IDT de muchas entradas*/
IDTR idtr;				/* IDTR */

PROCESS * createProcess(char* name, int (*process)(int,char**), int argc, char** argv, int priority, int isFront, int maxHeapPages, int maxStackPages);

extern unsigned int pageDirectory;
extern circularBuffer generalBuffer;
extern circularBuffer ioBuffer;
int tick = 0; /* utilizada por random */
int waitingTime = 0;
int clockRefresh = 18;
int shift = 0;
int capsLock = 0;
extern int nextPid;
extern PROCESS * currentProc;
extern int memSem;
TTY * ttys[4];
int currentTTY = 0;

void printTime() {
  int auxtickpos = ttys[currentTTY]->tickpos;
  ttys[currentTTY]->tickpos = 72*2;
  char auxformat = ttys[currentTTY]->format;
  ttys[currentTTY]->format = 0x70;
  int hour = _clock(04);
  int mins = _clock(02);
  int segs = _clock(00);
  char * video;
  video =  (char *) ttys[currentTTY]->outputBuffer;
  video[ttys[currentTTY]->tickpos] = ((hour&0xF0) >> 4) + '0'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = (hour&0x0F) + '0'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ':'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ((mins&0xF0) >> 4) + '0'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = (mins&0x0F) + '0'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ':'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ((segs&0xF0) >> 4) + '0'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = (segs&0x0F) + '0'; ttys[currentTTY]->tickpos++;
  video[ttys[currentTTY]->tickpos] = ttys[currentTTY]->format; ttys[currentTTY]->tickpos++;
  ttys[currentTTY]->format = auxformat;
  ttys[currentTTY]->tickpos = auxtickpos;
}


void
printBuffer(char * buffer) {
	int i;
	for ( i = 0 ; i< 512 ; i++ ) {
		printf("%d : ", (int)(buffer[i]) & 0x0ff);
	}
}


void
printasd(int a) {
	printf(" %d ", a);
}

void 
printa(char * a) {
	printf(" %d ", (int)(*a) & 0x0ff);
}

void printSomewhere(int position, char * str) {
  int i = 0;
  char * video;
  video = currentProc->tty->outputBuffer;
  while ( str[i] != '\0' ) {
    video[position + 2*i] = str[i];
    video[position + 2*i + 1] = 0x70;
    i++;
  }
}
void
printToScreen(int position, char * str) {
	int i = 0;
  char * video;
  video = (char *)0xb8000;
  while ( str[i] != '\0' ) {
    video[position + 2*i] = str[i];
    video[position + 2*i + 1] = 0x70;
    i++;
  }

}

/*TimerTick*/
void int_08() {
  tick++;
  currentProc->ticks++;
  //unblock(BLOCK_WAIT, NULL);  
  if ( clockRefresh == 0 ) {
    clockRefresh = 18;
    printTime();
  }
  unblock(BLOCK_WAIT, NULL);
  clockRefresh--;
  
}

char * backupBuffer;

/*Ingreso un dato por Teclado*/
void int_09(int scanCode) {
  if ( scanCode >= 0x3B && scanCode <= 0x3E ) {
	  int nextTTY;
	  nextTTY= scanCode - 0x3B;
	  memcpy(backupBuffer,(char*)0xb8000,80*2*25);
	  memcpy((char*)0xb8000,ttys[nextTTY]->outputBuffer,80*2*25);
	  ttys[currentTTY]->outputBuffer = backupBuffer;
	  backupBuffer = ttys[nextTTY]->outputBuffer;
	  ttys[nextTTY]->outputBuffer = (char*)0xb8000;
	  currentTTY = nextTTY;
	  return;
  }
  switch (scanCode) {
    case 54: case 42:
      shift = 1; break;
    case 58:
      capsLock = capsLock?0:1; break;
    case 170: case 182:
      shift = 0; break;
    default:
      if ( scanCode <= 126 ) {
	putInBuffer(scanCode, &(ttys[currentTTY]->inputBuffer), shift, capsLock);
      }
  }
}

void scroll() {
   char * video;
   video = (char *) currentProc->tty->outputBuffer;
   int i = 80*2*2;
   while ( i < 80*25*2 ) {
     video[i] = video[i+80*2];
     i++;
   }
   currentProc->tty->tickpos = 80*24*2;
   _cursor(currentProc->tty->tickpos/2);
}

void clear() {
   char * video;
   video =  (char *) currentProc->tty->outputBuffer;
   int i = 80*2*2;
   while ( i < 80*25*2 ) {
     video[i] = 0;
     i++;
   }
   currentProc->tty->tickpos = 80*2*2;
   _cursor(currentProc->tty->tickpos/2);
}

/*write*/
void int_80_0(int fd, void * buffer, int c){
  char * video;
  int i;
  switch(fd) {
    case SCREEN:
      video = currentProc->tty->outputBuffer;
      i = 0;
      while ( i < c ) {
	if ( *((int*)buffer + i) == '\n') {
	  currentProc->tty->tickpos +=  (( 80*2 ) - currentProc->tty->tickpos%(80*2));
	  if ( currentProc->tty->tickpos >= 80*25*2 ) {
	    scroll();
	  }
	} else if ( *((int*)buffer + i) == '\t' ) {
	  putchar(' ');
	  putchar(' ');
	} else if ( *((int*)buffer + i) == '\b' ) {
	  currentProc->tty->tickpos-=2;
	  putchar(' ');
	  currentProc->tty->tickpos-=2;
	   _cursor(currentProc->tty->tickpos/2);
	}
	else {
	  video[currentProc->tty->tickpos] = *((int*)buffer+i);
	  video[currentProc->tty->tickpos+1] = currentProc->tty->format;
	  currentProc->tty->tickpos+=2;
	  _cursor(currentProc->tty->tickpos/2);
	  if ( currentProc->tty->tickpos >= 80*25*2 ) {
	    scroll();
	  }
	}
	i++;
      }
      break;
    case SPEAKER:
      i = 0;
      while ( i < c ) {
	_Sti(); // Para habilitar el timer tick
	encodeChar(*((int*)buffer+i));
	i++;
      } break;
  }
}

void
yield(){
	asm("int $0x08");
}

/*read*/
void int_80_1(int fd, void * buffer, int c){
  //int res;
  int i;
  int character;
  switch(fd){
    case KEYBOARD:
      for(i=0 ; i<c; i++){
	if ( isEmpty(&ioBuffer) ) {
	  while ( (character = next(&(currentProc->tty->inputBuffer)) )  != '\n' ) {	  
	    if ( character != NULL ) {
	      if ( character == '\b' ) {
		removeLast(&ioBuffer);
	      } else {
		putCharInBuffer(character, &ioBuffer);
	      }
	    } else {
		    block(BLOCK_READ);
		    yield();
	    }
	  }
	  putCharInBuffer('\n', &ioBuffer);
	  reset(&(currentProc->tty->inputBuffer));
	} else {
		character = getFromBuffer(&ioBuffer);
		((int *)buffer)[i] = character; 
	}
      }
      break;
  }
}

void
int_00(){
  printf("ERROR : DIVISION BY ZERO."); 
}


void 
int_0e() {
	printToScreen(5*80*2, "PAPPA");
	return;
}



void int_80(int selector, int fd, void * buffer, int count) {
  switch(selector){
    case WRITE:
	int_80_0(fd, buffer, count);
	break;
    case READ:
	_Sti(); /*HABILITO LAS INT PARA ESPERAR DE TECLADO*/
	int_80_1(fd, buffer, count);
	break;
    case TICKPOS:
	*(int *)buffer = currentProc->tty->tickpos; break;
    case CURSOR:
	_cursor(count); break;
    case BACKSPACE:
	backspace(count); break;
    case PRINTSOMEWHERE:
	printSomewhere(fd, (char *)buffer); break;
  }
}

void backspace(int num) {
  if ( currentProc->tty->tickpos == num ) {
    putchar(' ');
    currentProc->tty->tickpos -= 2;
    _cursor(currentProc->tty->tickpos/2);
    return;
  }
  putchar('\b');
}

void waiti(int time) {
//printf("Entro a waiti\n");
  currentProc->waitingTime = time * 18;
  block(BLOCK_WAIT);
  yield();
}

int
idle(int argc, char** argv){
	while(1){
	}
	return 0;
}




void
printHeaders( Header * header){
	while ( header != NULL ) {
		printf("HEADER = %d / ", header);
		printf("STATUS = %d / ", header->status);
		printf("SIZE = %d / ", header->size);
		printf("NEXT = %d /> \n", header->next);
		header = header->next;
	}
}

//int useSector(int sector, char * buffer, int, int);
int
terminal( int argc, char ** argv ) {
	int num = stratoi(argv[0]);
	printf("Al.Fa.Jor S.O");
	printTime();
	printSomewhere(80*2+75*2, "ES_AR");
	printSomewhere(80*2+60*2, argv[0]);	
	ttys[num]->tickpos = 80*2*2;
	ENTRY root = { "/", 0};
	memcpy((char*)&(ttys[num]->cwd), (char*) &root, sizeof(ENTRY));
	memcpy((char*)&(ttys[num]->pcwd), (char*)&root, sizeof(ENTRY));

	char ** args = (char **)kmalloc( sizeof(char *) );
	char * arg0 = (char *)kmalloc( 2*sizeof(char) );
	strcpy(arg0, argv[0]);
	exec("shell", shell,  1, args, 2, 1, 100, 20);
	//printf("SIZEOF INODE = %d\n", sizeof(INODE));
	//printf("lalal");
	while(1) {

		block(BLOCK_WAITCHILD);
		yield();
	}
}

/**********************************************
kmain() 
Punto de entrada de cóo C.
*************************************************/

void kmain() 
{
  
  
	_Cli();
        _mascaraPIC2(0xFF);
	_mascaraPIC1(0xFC);

/* Borra la pantalla. */ 

	k_clear_screen();


/* CARGA DE IDT  */


        setup_IDT_entry (&idt[0x08], 0x08, (dword)&_int_08_hand, ACS_INT, 0); //Timer Tick
	
	setup_IDT_entry (&idt[0x09], 0x08, (dword)&_int_09_hand, ACS_INT, 0); //Teclado
	
	setup_IDT_entry (&idt[0x80], 0x08, (dword)&_int_80_hand, ACS_INT, 0);  // Linux
	
	setup_IDT_entry (&idt[0x0D], 0x08, (dword)&_int_00_hand, ACS_INT, 0);

	setup_IDT_entry (&idt[0x0E], 0x08, (dword)&_int_0e_hand, ACS_INT, 0);
	
/* Carga de IDTR    */

	idtr.base = 0;  
	idtr.base +=(dword) &idt;
	idtr.limit = sizeof(idt)-1;

	
	_lidt (&idtr);
	
/* Setea la paginacion y lo inicial del kernel */
	setPages();

	initialize(&ioBuffer);
/*	printf("Al.Fa.Jor S.O");
	printTime();
	printSomewhere(80*2+75*2, "ES_AR");
	tickpos = 80*2*2;*/
	//creo el proceso idle

	((Header *)(512*0x1000))->next = NULL;
	((Header *)(512*0x1000))->status = 1;
	PROCESS * idle_t = createProcess("idle", idle, 0, (char **)NULL, 4, 0, 1, 1); 
	//printToScreen(0,"creo idle_t");
	createProcSched(idle_t);
	//printToScreen(20,"creo sched");
	int i;
	for ( i = 0 ; i < 4 ; i++ ) {
		char name[15];
		char ** args = (char **)kmalloc( sizeof(char *) );
		char * arg0 = (char *)kmalloc( 2*sizeof(char) );
		args[0] = arg0;
		strcpy(name, "terminal");
		TTY * tty = kmalloc(sizeof(TTY));
		tty->outputBuffer = kmalloc( 80*25*2 );
		int j;
		for ( j = 0 ; j < 80*25*2 ; j++ ) {
			((char *)tty->outputBuffer)[j] = 0;
		}
		initialize(&(tty->inputBuffer));
		tty->format = 0x07;
		tty->tickpos = 0;
		charcat(name, '0' + i);
		strcpy(arg0, "");
		charcat(arg0, '0' + i);	
		PROCESS * ttyProc = createProcess(name, terminal, 1, args, 0, 0, 1, 1);
		ttyProc->tty = tty;
		insertProcess(ttyProc);
		ttys[i] = tty;
	}
	//printToScreen(10,"llego aca");
	backupBuffer = ttys[0]->outputBuffer;
 	ttys[0]->outputBuffer = (char*)0xb8000;
		
	//printf("IDLE_T = %d\n", idle_t);
	//creo el taskSched con idle
	
	init_fs();
	//printHeaders((Header*)(512*0x01000));
	//free((void *)(2097266+12));
//	printHeaders((Header*)(512*0x01000));	
//	while(1);
	//			
	//cree una shell
	//exec("shell", shell,  0, (char **)NULL, 2, 1, 100, 20);
	//printf("PAPA FRITA\n");
//	while(1);
	//exec("print1", print1, 1, 0, (char **)NULL, 2, 1, 1, 1);
	//exec("print2", print2, 1, 0, (char **)NULL, 2, 1, 1, 1);
	//Habilito interrupciones
	_Sti();
	//Incia con idle
	while(1) {
	}
	return;
}
int LoadStackFrame(int(*process)(int,char**),int argc,char** argv, int bottom, void(*cleaner)());



static void *
kmalloc2(int size, Header * p) {
	//printf("a");
	//Quizas haya que deshabilitar interrupciones.
	if ( p->next == NULL ) {
		//No hay lugar disponible
		if ( p->status == 0 ) {
			return NULL;
		}
		if ( (int)p + sizeof(Header) + size >= 1024*0x1000 ) {
			return NULL;
		} else {
			p->status = 0;
			p->size = size;
			if ( (int)p + 2 * sizeof(Header) + size >= 1024 * 0x1000 ) {
				p->next = NULL;
			} else {
				p->next = (Header*)((int)p + sizeof(Header) + size);
				p->next->status = 1;
				p->next->next = NULL;
			}
			return (void*)((int)p + sizeof(Header));
		}
	}
	if ( p->status == 1 ) {
		//printf("Entro aca?\n");
		if ( size >= p->size ) {
			return kmalloc2(size, p->next);
		} else {
			p->status = 0;	
			if ( size + sizeof(Header) < p->size ) {
			//	printf("AQUI ESTOY ENTRANDO\n");
			//	printf("SIZE = %d --- P->SIZE = %d\n", size, p->size);
				((Header *)((int)p + size + sizeof(Header)))->next = p->next;
			//printf("size-> %d\tpsizeee-> %d\tpneeext->%d\n",size,p->size,p->next);
			//	printf("P = %d\n", p);
			//	printf("NEW = %d, NEWSIZE = %d\n", (int)p + size + sizeof(Header), size + sizeof(Header)); 
				((Header *)((int)p + size + sizeof(Header)))->status = 1;
				((Header *)((int)p + size + sizeof(Header)))->size = p->size - size - sizeof(Header);
				p->next = (Header *)((int)p + size + sizeof(Header));
			}
			p->size = size;
			//printf("RETORNO POR ACA  --- %d\n", (int)p);
			//printf("RETORNO NEXT  --- %d\n", p->next);
			//printf("RETORNO NEXT->NEXT  --- %d\n", p->next->next);
				//printf("ALOCO: %d\n", p );

			return (void*)((int)p + sizeof(Header));
		}
	}
	return kmalloc2(size, p->next);
}


void *
kmalloc(int size){
	void * ret = kmalloc2(size, (void*)(512*0x00001000));
	return ret;

}

static void *
kcalloc(int cant, int size){

	void * p = kmalloc(cant*size);
	int i = 0;
	while ( i < cant*size ) {
		*(char*)(p + i) = 0;
		i++;
	}
	return p;		
}

int
getNextPid(){
	int aux = nextPid;
	nextPid++;
	return aux;
}


//Funcion que crea procesos
PROCESS * 
createProcess(char* name, int (*process)(int,char**), int argc, char** argv, int priority, int isFront, int maxHeapPages, int maxStackPages)
{
	PROCESS * newProcess = kmalloc(sizeof(PROCESS));
	

	//char* video =(char*) 0xb8000;
	int i;
	//newProcess->pid=GetPID();
	newProcess->pid = getNextPid();
	newProcess->foreground=isFront;
	newProcess->priority=priority;
	memcpy(newProcess->name,name,strlen(name)+1);
	newProcess->blocked=NOT_BLOCKED;
	if ( currentProc != NULL ) {
		newProcess->tty=currentProc->tty;
	}
	newProcess->lastCalled=0;
	//newProcess->ESP=LoadStackFrame(process,argc,argv,(int)(stack+stacklength-1),Cleaner);
	/*if(isFront && CurrentPID>=1)
	{
		PROCESS* proc=GetProcessByPID(CurrentPID);
		char Men[10];
		proc->blocked=2;
		procesos[i].parent=CurrentPID;
	}*/
	//video[2080]=procesos[i].pid+0x30
	
	if( currentProc != NULL ){/*Caso de creacion del idle*/
		newProcess->parentPid = currentProc->pid;
	}else{
		newProcess->parentPid = -1; 
	}

	int ficha = 0, k, j;
	for(i = 2 ; i < 4; i++){
		for( j = 0 ; j < 1024 ; j++ ){
			if ( ( P_DIR(pageDirectory,i,j) & 0x226) == 0x026 ) {
				if ( ficha == 0 ) {
					newProcess->firstHeapPage = newProcess->lastHeapPage = (void *)( P_DIR(pageDirectory, i, j) & 0xFFFFF000);
					newProcess->heapPages = kcalloc(maxHeapPages, sizeof(void *));
					newProcess->heapPages[0] = newProcess->firstHeapPage;
					((Header *)newProcess->firstHeapPage)->status = 1;
					((Header *)newProcess->firstHeapPage)->next = NULL;
					newProcess->maxHeapPages = maxHeapPages;
					ficha++;
				//	P_DIR(pageDirectory,i,j) &= 0xfffffffe;
					P_DIR(pageDirectory,i,j) |= 0x227;
				} else {
					int lasti, lastj;
					for ( k = 0 ; k < maxStackPages ; k++, j++ ) {
						if ( j == 1024 ) {
							i++;
							if ( i == 4 ) {
								return NULL;
							}
							j = 0;
						}
						P_DIR(pageDirectory,i,j) |= 0x227;
						lasti = i;
						lastj = j;
					}
					if ( k == maxStackPages ) { //TODO CORREGIR LIBERAR PAGS QUE OCUPA SIN VER QUE TENGA TODO EL ESPACIO QUE QUIERE
						newProcess->stacksize = 0x1000 * maxStackPages;
						newProcess->stackstart = ( P_DIR(pageDirectory, lasti, lastj) & 0xFFFFF000 ) + newProcess->stacksize -sizeof(STACK_FRAME) ;
						newProcess->ESP=LoadStackFrame(process,argc,argv, newProcess->stackstart + sizeof(STACK_FRAME),Cleaner);
						return newProcess;

					}
			  	}
			}
		}
	}
	return NULL;
}



//Funcion que arma los stack frames de cada proceso
int 
LoadStackFrame(int(*process)(int,char**),int argc,char** argv, int bottom, void(*cleaner)())
{
	//printf("BOTTOM: %d\n", bottom);
	STACK_FRAME* frame= (STACK_FRAME*)(bottom-sizeof(STACK_FRAME));
	frame->EBP=0;
	frame->EIP=(int)process;
	frame->CS=0x08;
	
	frame->EFLAGS=0x0200;
	frame->retaddr=cleaner;
	frame->argc=argc;
	frame->argv=argv;
	return (int)frame;
}

void
exec(char* name, int (*process)(int,char**), int argc, char** argv, int priority, int isFront, int maxHeapPages, int maxStackPages){
	//if( argc == 2)
	//	printf("PID = -%s-\n AUX = -%s-\n", argv[0], argv[1]);
	PROCESS * proc = createProcess(name, process, argc, argv, priority, isFront, maxHeapPages, maxStackPages);
	insertProcess(proc);	
	return;
}

void 
destroy(PROCESS * proc)
{
  	//printf("Entre en destroy\n");
	if( proc->foreground ){
		unblock(BLOCK_WAITCHILD, NULL);
	//	printf("desblokeo por child");
	}
	free(proc->heapPages);
	//TODO LIBERAR PAGS DEL STACK Y HEAP
	free(proc); //funcion de sched
	return;
}

static inline
void outb( unsigned short port, unsigned char val )
{
    asm volatile( "outb %0, %1"
                  : : "a"(val), "Nd"(port) );
}

static inline
unsigned char inb( unsigned short port )
{
    unsigned char ret;
    asm volatile( "inb %1, %0"
                  : "=a"(ret) : "Nd"(port) );
    return ret;
}

static inline
void outw( unsigned short port, unsigned short val )
{
    asm volatile( "outw %0, %1"
                  : : "a"(val), "Nd"(port) );
}

static void
writeSector(unsigned char * buffer, int count) {
	int i;
	short tmpword;
	for (i = 0; i < count; i += 2 ) {
		tmpword = (buffer[(i+1)] << 8) | buffer[i];
		//printf("%d\n", (int)tmpword & 0x00ffff);
		outw(0x1f0, tmpword);
	}
}

int
useSector(int sector, char * buffer, int cmd, int count) {
	outb(0x1f6, 0x0e0 | ((sector >> 24) & 0x0f));
	outb(0x1f1, 0x00);
	outb(0x1f2, (unsigned char) count);
	outb(0x1f3, (unsigned char) sector);
	outb(0x1f4, (unsigned char)( sector >> 8));
	outb(0x1f5, (unsigned char)( sector >> 16));

	if ( cmd == READ ) { 
		outb(0x1f7, 0x20);
	} else if ( cmd == WRITE ) {
		outb(0x1f7, 0x30);
	} else {
		return -1;
	}
	unsigned char status;
	//int i = 0;
	while ( 1 ) {
	//	i = 0;
		status = inb(0x1f7);
		while ( (status & 0x80) || !(status & 0x08) ) {
			status = inb(0x1f7);
		}
		if ( !(status & 0x80) && (status & 0x08) ) {
			if ( cmd == READ ) {
			  	//waiti(1);
				_readSector(buffer, count * 256);
				return 0;
			} else {
			  	//waiti(1);
				writeSector(buffer, count * 512);
				outb(0x1f7, 0xe7);
				return 0;
			}
		} 
		if ( (status & 0x01) == 0x01 || (status & 0x20) == 0x20  ) {
			//printf("asd\n");
			//printf("%d\n", status);
			return -1;
		}
	}
}
