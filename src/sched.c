#include "../include/defs.h"
#include "../include/kasm.h"
#include "../include/libstd.h"
#include "../include/QueueADT.h"
#include "../include/kernel.h"
#include "../include/ListADT.h"
#include "../include/mempages.h"
#include "../include/string.h"

#define P_DIR(dir, i, j) ( ( (unsigned int *) ( ( unsigned int ) ( ( ( unsigned int * ) pageDirectory )[i] ) & 0xfffff000 ) )[j] )
extern int tickpos;
extern PROCESS * currentProc;
extern unsigned int pageDirectory;

/*Shceduler por round-robin*/
PROCESS * GetNextTask();
int last100[100]={0};
int counter100;
int FirstTime=1;
//extern PROCESS idle;
int nextPid = 0;
SEMAPHORE removeSem = DOWN;

PROCESS * idle_t;
PROCESS * currentProc = NULL;
QueueADT processes;
ListADT *  blockedProcs;

#define	STOP	_Cli();while(1);

/*Crea el shceduler y lo incializa con el proceso idle*/
void
createProcSched(PROCESS * newidle_t){
	currentProc = newidle_t;
	processes = newQueue();
	blockedProcs = newListADT(NULL, NULL);
	idle_t = newidle_t;
	return;
}

int
printAllPids(int argc, char ** argv){
	QIterator * it = kmalloc( queueIteratorSize() );
	queueIteratorReset( processes, it);
	PROCESS * p;

	printf("\nProcesos acutales: ( name | pid | parentpid ) \n");
	
	if( idle_t != NULL )
		printf("idle\t%d\t%d\n",idle_t->pid, idle_t->parentPid);
	
	while( ( p = (PROCESS *)queueNext(it) ) != NULL ){
		printf("%s\t%d\t%d\n", p->name, p->pid, p->parentPid);	
	}
	free(it);
  
	Iterator * itBlocked = kmalloc( iteratorSize() );
	listIteratorReset( blockedProcs, itBlocked);
	//printf("(Blocked) size > %d\t", getSize(blockedProcs) );
	while( ( p = (PROCESS *)listNext(blockedProcs,itBlocked) ) != NULL ){
		printf("%s(blocked)\t%d\t%d\n", p->name, p->pid, p->parentPid);
	}
	free(itBlocked);
	return 0;
}


/*Introduce al scheduler la tarea task*/
void
insertProcess(PROCESS * proc){
 // printf("Craga el...");
  //print_proc_gdb(proc);
//	printf("Inserto %s\n", proc->name);
	enqueue( processes, proc);
	return;
}

int removeProcess(PROCESS * proc);
ListADT * getChildsPids(int);

int removeProcessByPid2(int pid);

int
removeProcessByPid(int pid){
	removeProcessByPid2(pid);
	//repeat en los blokeados
	return 0;
}

int
removeProcessByPid2(int pid){
  QIterator * it = kmalloc( queueIteratorSize() );
  queueIteratorReset( processes, it);
  PROCESS * p;
  int * childPid;
  ListADT * childPids;
  Iterator * childsIt;
  while( ( p = (PROCESS *)queueNext(it) ) != NULL ){
    if( pid == p->pid ){
      removeProcess(p);
      childPids = getChildsPids(pid);
      childsIt = kmalloc( iteratorSize() );
      listIteratorReset( childPids, childsIt);
      childPid = listNext( childPids, childsIt);
      //iterar por la lista
      //printf("hay hijos: %d",*childPid);
      //while(1)
      //	_Cli();
      
      while( childPid != NULL ){
	removeProcessByPid2( *childPid);
	childPid = listNext( childPids, childsIt);
      }
      freeList(childPids);
      free(childsIt);
      free(it);
      return 0;
    }
  }
  
  Iterator * itBlocked = kmalloc( iteratorSize() );
  listIteratorReset( blockedProcs, itBlocked);
  while( ( p = (PROCESS *)listNext(blockedProcs,itBlocked) ) != NULL ){
    if( pid == p->pid ){
      removeProcess(p);
      childPids = getChildsPids(pid);
      childsIt = kmalloc( iteratorSize() );
      listIteratorReset( childPids, childsIt);
      childPid = listNext(blockedProcs,childsIt);
      //iterar por la lista
      while( childPid != NULL ){
	removeProcessByPid2( *childPid );
	childPid = listNext( childPids, childsIt);
      }
      freeList(childPids);
      free(itBlocked);
      free(childsIt);
      return 0;
    }
  }
  return 0;
}

ListADT *
getChildsPids(pid){

	ListADT * ans = newListADT(NULL, free);

	int * new;
	/* Itero por la cola de procesos acitvos y los agergo a ans si son hijos de pid */
	QIterator * it = kmalloc( queueIteratorSize() );
	queueIteratorReset( processes, it);
	PROCESS * p;
	while( ( p = (PROCESS *)queueNext(it) ) != NULL ){
		//printf("observa el %s-%d ", p->name, p->pid);
		if( pid == p->parentPid ){
			new = (int *)kmalloc( sizeof(int) );
			(*new) = p->pid;
			add( ans, new);
		}
	}
	//while(1)
	//  _Cli();
	/* Itero por la lista de los blokeados y los agrego a ans si son hijos de pid*/
	Iterator * itBlocked = kmalloc( iteratorSize() );
	listIteratorReset( blockedProcs, itBlocked);
	while( ( p = (PROCESS *)listNext(blockedProcs,itBlocked) ) != NULL ){
		if( pid == p->parentPid ){
			new = (int *)kmalloc( sizeof(int) );
			(*new) = p->pid;
			add( ans, new);
		}
	}
	free(itBlocked);
	free(it);
	return ans;
}

int
removeProcess(PROCESS * proc){
	while(removeSem == UP){
		;
	}
	removeSem = UP;
	removeFromQueue( processes, proc);
	Iterator * it = kmalloc(iteratorSize());
	listIteratorReset(blockedProcs, it);
	PROCESS * nextProc;
	int i = 0;
	while ( ( nextProc = listNext(blockedProcs, it)) != NULL ) {
		if ( nextProc == proc ) {
			removeFromListI(blockedProcs, i);
			break;
		}
		i++;
	}
	removeSem = DOWN;
	return 0;
}

void
resetCurrentProc(){
	currentProc = NULL;
	return;
}

//Funcion que almacena el ESP actual
void SaveESP (int ESP)
{
	PROCESS* temp;
	if (!FirstTime && currentProc!=NULL)
	{
	//	printf("NEW ESP: %d\n", ((STACK_FRAME*)ESP));
	//	printf("CURRENT PROC ESP: %d\n", ((STACK_FRAME*)currentProc->ESP));
		temp=currentProc;
		temp->ESP=ESP;	}
	FirstTime=0;
	return;
}

//Funcion que obtiene el ESP de idle para switchear entre tareas.
void* GetTemporaryESP (void)
{
	return (void*)idle_t->ESP;
}

//Funcion que devuelve el PROCESS* siguiente a ejecutar
PROCESS* GetNextProcess(void)
{
	//char* video=(char*)0xb8000;
	PROCESS* temp;
	//selecciona la tarea
	temp = GetNextTask();
	//printf("Next Proc\n");
//	printf("DSP DE GETNEXTTASK ESP: %d\n", ((STACK_FRAME*)temp->ESP));
	//printf("BABABBABA\n");
	return temp;
}

//Funcion Scheduler
PROCESS *
GetNextTask()
{
	if ( currentProc != NULL && currentProc != idle_t ) {
	  //printf("CURRENT PROCESS - %s = %d\n", currentProc->name, currentProc);
	 // printf("Craga ejl...");
	  //print_proc_gdb(currentProc);
		if ( currentProc->status != BLOCKED ) {
			currentProc->status = READY;
			enqueue( processes, currentProc);
		} else {
			add(blockedProcs, currentProc);
		}
	}
	if(queueIsEmpty(processes)){
		currentProc = idle_t;
		idle_t->ticks++;
		idle_t->status = RUNNING;
		return idle_t;
	} else {
		idle_t->status = READY;
	}
	currentProc = dequeue(processes);
	
	//currentProc->ticks++;
	currentProc->status = RUNNING;
	//printf("Saca el...");
	//print_proc_gdb(currentProc);
	return currentProc;
}

//Funcion que devuelve el ESP del proceso actual.
int LoadESP(PROCESS* proc)
{
  //printf("Estoy loadeando\n");
	//printf("%d\n", tickpos);
//	printf("ASD");
	return proc->ESP;
}

//Funcion a donde van a parar las funciones que terminan.
void Cleaner(void)
{
	//char Men[10];
	_Cli();
	destroy(currentProc);
	currentProc = NULL;
	//Destroy(currentProc);
	//k_clear_screen();
	_Sti();
	while(1);
}

void block(int status) {
	currentProc->status = BLOCKED;
	currentProc->blocked = status;
	//add(blockedProcs, currentProc);
	//printf("se blokeo a %s\n",currentProc->name);
}

void unblock(int status, TTY * tty) {
	//printf("Entro al unblock");
	Iterator * it = kmalloc(iteratorSize());
	listIteratorReset(blockedProcs, it);
	int i = 0;
	PROCESS * proc;
	//printf("unblock - A ver si llego");
	while ( (proc = listNext(blockedProcs, it)) != NULL ) {
		//printf("%d\n", tty);
		if ( proc->blocked == status ){
			switch( status ){
			  case BLOCK_WAIT:
				if( proc->waitingTime == 0 ){
					removeFromListI(blockedProcs, i);
					proc->status = READY;
					enqueue( processes, proc);
				}else{
					proc->waitingTime--;
				}
				break;
			  case BLOCK_WAITCHILD:
				if( currentProc->parentPid == proc->pid ){
					removeFromListI(blockedProcs, i);
					proc->status = READY;
					enqueue( processes, proc);
				//	printf("se desblokeo a %s",proc->name);
				}
				break;
			  case BLOCK_READ:
				if ( tty == proc->tty ) {
					//printf("removiendo %d\n", i);
					removeFromListI(blockedProcs, i);
					proc->status = READY;
					enqueue(processes, proc);
					goto end;
				} break;
			  default:
				removeFromListI(blockedProcs, i);
				proc->status = READY;
				enqueue( processes, proc);
			//	printf("se desblokeo a %s",proc->name);
			}
		}
		i++;
	}
end:
	free(it);
		//	printf("PAPA GUISADA\n");

}

typedef struct {
	char name[MAX_PROCESS_NAME];
	int ticks;
	int state;
	int blocked;
} TopStruct;

void
resetTicks(){	
	QIterator * qit = kmalloc(queueIteratorSize());
	Iterator * lit = kmalloc(iteratorSize());
	PROCESS * proc;
	queueIteratorReset(processes, qit);
	while ( (proc = queueNext(qit)) != NULL ) {
		proc->ticks = 0;
	}
	listIteratorReset(blockedProcs, lit);
	while ( (proc = listNext(blockedProcs, lit)) != NULL ) {
		proc->ticks = 0;
	}
	idle_t->ticks = 0;
	if ( currentProc != idle_t ) {
		currentProc->ticks = 0;
	}
	free(qit);
	free(lit);
	return;
}


int
ktop(int argc, char ** argv) {
	QIterator * qit = kmalloc(queueIteratorSize());
	Iterator * lit = kmalloc(iteratorSize());
	PROCESS * proc;
	TopStruct array[100]; 
	int sum;
	int i, last;
	while(1) {
		resetTicks();
		
		waiti(2); 
		last = 0;
		sum = 0;
		i = 0;
		
		queueIteratorReset(processes, qit);
		while ( (proc = queueNext(qit)) != NULL ) {
			sum += proc->ticks;
			strcpy(array[i].name, proc->name);
			array[i].ticks = proc->ticks;
			array[i].state = proc->status;
			i++;
		}
		
		
		listIteratorReset(blockedProcs, lit);
		while ( (proc = listNext(blockedProcs, lit)) != NULL ) {
			sum += proc->ticks;
			strcpy(array[i].name, proc->name);
			array[i].ticks = proc->ticks;
			array[i].state = proc->status;
			array[i].blocked = proc->blocked;
			i++;
		}
		sum += idle_t->ticks;
		strcpy(array[i].name, idle_t->name);
		array[i].ticks = idle_t->ticks;
		array[i].state = idle_t->status;
		i++;
		if ( currentProc != idle_t ) {
			sum += currentProc->ticks;
			strcpy(array[i].name, currentProc->name);
			array[i].ticks = currentProc->ticks;
			array[i].state = currentProc->status;
		}
		
		last = i+1;
		int w;
		int y;
		for ( w = 0 ; w < last ; w++ ) {
			for ( y = 0 ; y < last - w ; y++ ) {
				if ( array[y].ticks < array[y+1].ticks ) {
					TopStruct aux = array[y];
					array[y] = array[y+1];
					array[y+1] = aux;
				}
			}
		}
		clear();
		printf("TOP\n");
		printf("PROGRAM NAME\tCPU%%\tState\n");
		char * state_str;
		for ( i = 0 ; i < last ; i++ ) {
			printf("%s\t", array[i].name);
			int percentage = array[i].ticks*100;
			percentage /= sum;
			switch( array[i].state ){
			  case RUNNING:
			    state_str = "Running";break;
			  case READY: 
			    state_str = "Ready";break;
			  case BLOCKED:
			    state_str = "Blocked"; break;
			}
			printf("%d%%%s\t\n", percentage, state_str);
		}
	}
	return 0;
}
