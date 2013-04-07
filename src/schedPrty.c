#include "../include/defs.h"
#include "../include/kasm.h"
#include "../include/libstd.h"
#include "../include/QueueADT.h"
#include "../include/kernel.h"
#include "../include/ListADT.h"
#include "../include/string.h"
#include "../include/mempages.h"

/* Scheduler de prioridades, del 1 al 4 inclusive */


#define	CANT_PRIORITYS 5

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
QueueADT processes[CANT_PRIORITYS];
ListADT *  blockedProcs;

#define	STOP	_Cli();while(1);

/*Crea el shceduler y lo incializa con el proceso idle*/
void
createProcSched(PROCESS * newidle_t){
	int i;
	currentProc = newidle_t;
	for( i = 0; i < CANT_PRIORITYS; i++){
		processes[i] = newQueue();
		//printf("SCHED > COLA %i : %d\n", i, processes[i]);
	}
	blockedProcs = newListADT(NULL, NULL);
	idle_t = newidle_t;
	return;
}

void
printAllPids(){
	QIterator * it = kmalloc( queueIteratorSize() );
	
	PROCESS * p;
  
	printf("\nProcesos acutales: ( name | pid | parentpid ) \n");
  
	if( idle_t != NULL )
		printf("idle\t%d\t%d\n",idle_t->pid, idle_t->parentPid);
	
	int i = 0;
	for( i = 0; i < CANT_PRIORITYS; i++){
		queueIteratorReset( processes[i], it);
		while( ( p = (PROCESS *)queueNext(it) ) != NULL ){
			printf("%s\t%d\t%d\n", p->name, p->pid, p->parentPid);	
		}
	}
	free(it);

	Iterator * itBlocked = kmalloc( iteratorSize() );
	listIteratorReset( blockedProcs, itBlocked);
	//printf("Blokeados size > %d\t", getSize(blockedProcs) );
	while( ( p = (PROCESS *)listNext(blockedProcs,itBlocked) ) != NULL ){
		printf("%s(blocked)\t%d\t%d\n", p->name, p->pid, p->parentPid);
	}
	free(itBlocked);
	return;
}


static void
print_proc_gdb(PROCESS * proc){
	printf("/-- PROCESO %s -- fun : %d --/\n", proc->name, ((STACK_FRAME *)(proc->ESP))->EIP);
}

/*Introduce al scheduler la tarea task*/
void
insertProcess(PROCESS * proc){
 // printf("Craga el...");
  //print_proc_gdb(proc);
	enqueue( processes[(proc->priority)], proc);
	//printf("Inserto %d(%d) en cola %d\n", proc, proc->priority, processes[(proc->priority)]);
	return;
}

int removeProcess(PROCESS * proc);
ListADT * getChildsPids(int);

int
removeProcessByPid(int pid){
  QIterator * it = kmalloc( queueIteratorSize() );
  
  PROCESS * p;
  int * childPid;
  ListADT * childPids;
  Iterator * childsIt;
  int i = 0;
  
  for( i=0; i<CANT_PRIORITYS; i++ ){
	queueIteratorReset( processes[i], it);
	while( ( p = (PROCESS *)queueNext(it) ) != NULL ){
		if( pid == p->pid ){
			removeProcess(p);
			childPids = getChildsPids(pid);
			childsIt = kmalloc( iteratorSize() );
			listIteratorReset( childPids, childsIt);
			childPid = listNext( childPids, childsIt);
			while( childPid != NULL ){
				removeProcessByPid( *childPid);
				childPid = listNext( childPids, childsIt);
			}
			free(childPids);
			free(childsIt);
			free(it);
			return 0;
		}
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
	removeProcessByPid( *childPid );
	childPid = listNext( childPids, childsIt);
      }
      free(childPids);
      free(itBlocked);
      free(childsIt);
      return 0;
    }
  }
  
  //repeat en los blokeados
  printf("PID no hallado. Funcion no exitosa.\n");
  return -1;
}

ListADT *
getChildsPids(pid){
  ListADT * ans = newListADT(NULL, NULL);
  int * new;
  /* Itero por la cola de procesos acitvos y los agergo a ans si son hijos de pid */
  QIterator * it = kmalloc( queueIteratorSize() );
  PROCESS * p;
  int i = 0;
  
  for( i=0; i<CANT_PRIORITYS; i++ ){
    queueIteratorReset( processes[i], it);
    while( ( p = (PROCESS *)queueNext(it) ) != NULL ){
      if( pid == p->pid ){
	new = (int *)kmalloc( sizeof(int) );
	(*new) = pid;
	add( ans, new);
      }
    }
  }
  /* Itero por la lista de los blokeados y los agrego a ans si son hijos de pid*/
  Iterator * itBlocked = kmalloc( iteratorSize() );
  listIteratorReset( blockedProcs, itBlocked);
  while( ( p = (PROCESS *)listNext(blockedProcs,itBlocked) ) != NULL ){
    if( pid == p->pid ){
      new = (int *)kmalloc( sizeof(int) );
      (*new) = pid;
      add( ans, new);
    }
  }
  
  return ans;
}

int
removeProcess(PROCESS * proc){
	while(removeSem == UP){
		;
	}
	removeSem = UP;
	removeFromQueue( processes[(proc->priority)], proc); //Implementar
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
	//printf("llego a saveesp\n");
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
	//printf("llego a gettempesp\n");
	return (void*)idle_t->ESP;
}

//Funcion que devuelve el PROCESS* siguiente a ejecutar
PROCESS* GetNextProcess(void)
{
	//char* video=(char*)0xb8000;
	PROCESS* temp;
	//selecciona la tarea
	//printToScreen(40,"entro a get");
	temp = GetNextTask();
	//printToScreen(50,"salio de get");
//	printf("DSP DE GETNEXTTASK ESP: %d\n", ((STACK_FRAME*)temp->ESP));
	//printf("BABABBABA\n");
	return temp;
}

int getNextPriority( int * queuesData, int rand);
void getPercents( int * queuesData );

//Funcion Scheduler
PROCESS *
GetNextTask()
{
	if ( currentProc != NULL && currentProc != idle_t ) {
		enqueue( processes[(currentProc->priority)], currentProc);
	}
	//printToScreen(0,"AA");
	
	/* Algoritmo de sheduling con prioridades */
	int n;
	int p;
	
	/* Si no hay ningun proceso, se devuelve el idle */
	int i, allEmpty = 1;
	int queuesData[CANT_PRIORITYS] = {0};
	for( i = 0; i < CANT_PRIORITYS; i++){
		if( !queueIsEmpty(processes[i]) ){
			queuesData[i] = 1;
			allEmpty = 0;
		}
	}
	//printToScreen(0,( allEmpty == 0 )?"BB":"AA");
	//while(1)
	//_Cli();
	if( allEmpty ){
		currentProc = idle_t;
		return idle_t;
	}
	//printToScreen(0,"ASD");
	/* Se pide un numero al azar entre [0,100) */
	n = random();
	//printToScreen(10,( n > 50 )?"1":"0");
	
	/* Se mapea el numero al azar a la prioridad respectiva */
	p = getNextPriority( queuesData, n);
	
	/* Se desencola el proximo proceso de esa prioridad */
	currentProc = dequeue(processes[p]);

	return currentProc;
}

/*   ----  Funciones de Scheduling --------  */
int
getNextPriority( int * queuesData, int rand){
	/* Decodifico queueData al porcentaje de n correspondiente a esa prioridad */
	getPercents(queuesData);
	
	int i;
	for( i = 0; i < CANT_PRIORITYS; i++){
		if( rand < queuesData[i] ){
			return i;
		}
	}
	
	/*Jamas llegaria a este punto*/
	return NULL;
}

int
cantQueuesAvailables(int * vec){
	int cant = 0;
	int i;
	for( i=0; i<CANT_PRIORITYS; i++){
		cant += vec[i];
	}	
	return cant;
}

void
getPercents( int * vec ){
	/*K: Cte. que depende del numero de colas con procesos*/
	int k, cantQueues;
	switch( cantQueues = cantQueuesAvailables(vec) ){
		case 1:
			k = 10; break;
		case 2:
			k = 11; break; 
		case 3:
			k = 8; break;
		case 4:
			k = 6; break;
		case 5:
			k = 6; break;
	}
  
	int i, sum  = 0;
	for( i=0; i < CANT_PRIORITYS; i++){
		if( vec[i] != 0 ){
		  vec[i] = k*(CANT_PRIORITYS-i);
			sum += vec[i];
		}
	}
  
	if(sum > 100)
		printf("cambia los datos papaa");
  
	int resta;	
	int acum; 
	int firstTime = 1;

	if(sum < 100){
		//printf("(suma = %d)", sum);
		resta = 100 - sum;
		//printf("(suma = %d)", resta);
		acum = resta / cantQueues;
		//printf("(acum = %d)", acum);
		resta = resta % cantQueues;
		for( i = 0; i<CANT_PRIORITYS; i++){
			if( vec[i] != 0 ){
				vec[i] += acum;
				if( firstTime ){
					vec[i] += resta;
					firstTime = 0;
				}
			}
		}
	}
  
	int j;
	for( i=CANT_PRIORITYS-1; i>=0; i--){
		if( vec[i] != 0 ){
			for( j=0; j<i; j++){
				vec[i] += vec[j];
			}
		}
	}
}

/*----------------------------------------*/

//Funcion que devuelve el ESP del proceso actual.
int LoadESP(PROCESS* proc)
{
  //printf("Estoy loadeando\n");
	//printf("%d\n", tickpos);
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

void 
block(int status) {
	currentProc->status = BLOCKED;
	currentProc->blocked = status;
	add(blockedProcs, currentProc);
}

void unblock(int status, TTY * tty) {
	Iterator * it = kmalloc(iteratorSize());
	listIteratorReset(blockedProcs, it);
	int i = 0;
	PROCESS * proc;
	//printf("unblock - A ver si llego");
	while ( (proc = listNext(blockedProcs, it)) != NULL ) {
		//printf("%d\n", tty);
		if ( proc->blocked == status /*&& ( tty == NULL || tty  == proc->tty ) */){
			switch( status ){
			  /*case BLOCK_WAIT:
				if( proc->waitingTime == 0 ){
					removeFromListI(blockedProcs, i);
					enqueue( processes, proc);
					printf("se desblokeo a %s",proc->name);
				}else{
					proc->waitingTime--;
				}
				break;*/
			  case BLOCK_WAITCHILD:
				if( currentProc->parentPid == proc->pid ){
					removeFromListI(blockedProcs, i);
					insertProcess(proc);
					//printf("se desblokeo a %s",proc->name);
				}
				break;
			  case BLOCK_READ:
				if( tty == NULL || tty  == proc->tty ){
					removeFromListI(blockedProcs, i);
					insertProcess(proc);
					//printf("se desblokeo a %s",proc->name);
				}
			  default:
				removeFromListI(blockedProcs, i);
				insertProcess(proc);
				//printf("se desblokeo a %s",proc->name);
			}
			if ( status == BLOCK_READ && ( tty == NULL || tty  == proc->tty ) ) {
				break;
			}
		}
		i++;
	}
	free(it);
}


typedef struct {
	char name[MAX_PROCESS_NAME];
	int ticks;
	int state;
} TopStruct;

void
resetTicks(){	
	QIterator * qit = kmalloc(queueIteratorSize());
	Iterator * lit = kmalloc(iteratorSize());
	PROCESS * proc;
	int i;
	
	for( i = 0; i < CANT_PRIORITYS; i++){
		queueIteratorReset(processes[i], qit);
		while ( (proc = queueNext(qit)) != NULL ) {
			proc->ticks = 0;
		}
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
	//printf("ENTRE EN EL PUTO TOP\n");
	QIterator * qit = kmalloc(queueIteratorSize());
	Iterator * lit = kmalloc(iteratorSize());
	PROCESS * proc;
	TopStruct ** array; 
	int sum;
	int i, j, qSize;
	int i_priority = 0;
	
	while(1) {
		resetTicks();
		
		//waiti(1); descomentar cuando se descomente waiti
		qSize = 0;
		for( j=0; j < CANT_PRIORITYS; j++)
			qSize += queueSize( processes[j] );
		
		array = kmalloc( ( qSize + getSize(blockedProcs) + 2) * sizeof(TopStruct*) ); 
		sum = 0;
		i = 0;
		
		for( i_priority = 0; i_priority < CANT_PRIORITYS; i_priority++){
			queueIteratorReset(processes[i_priority], qit);
			while ( (proc = queueNext(qit)) != NULL ) {
				sum += proc->ticks;
				TopStruct * entry = kmalloc(sizeof(TopStruct));
				strcpy(entry->name, proc->name);
				entry->ticks = proc->ticks;
				entry->state = proc->blocked;
				array[i++] = entry;
			}
		}
		
		
		listIteratorReset(blockedProcs, lit);
		while ( (proc = listNext(blockedProcs, lit)) != NULL ) {
			sum += proc->ticks;
			TopStruct * entry = kmalloc(sizeof(TopStruct));
			strcpy(entry->name, proc->name);
			entry->ticks = proc->ticks;
			entry->state = proc->blocked;
			array[i++] = entry;
		}
		
		
		sum += idle_t->ticks;
		
		TopStruct * entry = kmalloc(sizeof(TopStruct));
		strcpy(entry->name, idle_t->name);
		entry->ticks = idle_t->ticks;
		entry->state = idle_t->blocked;
		array[i] = entry;
		i++;
		if ( currentProc != idle_t ) {
			sum += currentProc->ticks;
			TopStruct * entry = kmalloc(sizeof(TopStruct));
			strcpy(entry->name, currentProc->name);
			entry->ticks = currentProc->ticks;			
			entry->state = currentProc->blocked;
			array[i] = entry;
		}
		
		int last = i+1;
//		clear();
		printf("TOP\n");
		printf("PROGRAM NAME\tCPU%%\tState\n");
		char * state_str;
		for ( i = 0 ; i < last ; i++ ) {
			printf("%s\t", array[i]->name);
			int percentage = array[i]->ticks;
			switch( array[i]->state ){
			  case RUNNING:
			    state_str = "Blocked";break;
			  case READY: 
			    state_str = "Ready";break;
			  case BLOCKED:
			    state_str = "Blocked"; break;
			}
			printf("%d.%d\t%s\n", ((int)percentage), sum, state_str); 
		}
		free(array);
	}
}
/*
esto es de sched.c

typedef struct {
	char name[MAX_PROCESS_NAME];
	int ticks;
	int state;
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
	printf("ENTRE EN EL PUTO TOP\n");
	QIterator * qit = kmalloc(queueIteratorSize());
	Iterator * lit = kmalloc(iteratorSize());
	PROCESS * proc;
	TopStruct ** array; 
	int sum;
	int i;
	while(1) {
		resetTicks();
		
		//waiti(1); descomentar cuando este waiti descomentado
		
		array = kmalloc( (queueSize(processes) + getSize(blockedProcs) + 2) * sizeof(TopStruct*) ); 
		sum = 0;
		i = 0;
		
		queueIteratorReset(processes, qit);
		while ( (proc = queueNext(qit)) != NULL ) {
			sum += proc->ticks;
			TopStruct * entry = kmalloc(sizeof(TopStruct));
			strcpy(entry->name, proc->name);
			entry->ticks = proc->ticks;
			entry->state = proc->blocked;
			array[i++] = entry;
		}
		
		
		listIteratorReset(blockedProcs, lit);
		while ( (proc = listNext(blockedProcs, lit)) != NULL ) {
			sum += proc->ticks;
			TopStruct * entry = kmalloc(sizeof(TopStruct));
			strcpy(entry->name, proc->name);
			entry->ticks = proc->ticks;
			entry->state = proc->blocked;
			array[i++] = entry;
		}
		
		
		sum += idle_t->ticks;
		
		TopStruct * entry = kmalloc(sizeof(TopStruct));
		strcpy(entry->name, idle_t->name);
		entry->ticks = idle_t->ticks;
		entry->state = idle_t->blocked;
		array[i] = entry;
		i++;
		if ( currentProc != idle_t ) {
			sum += currentProc->ticks;
			TopStruct * entry = kmalloc(sizeof(TopStruct));
			strcpy(entry->name, currentProc->name);
			entry->ticks = currentProc->ticks;			
			entry->state = currentProc->blocked;
			array[i] = entry;
		}
		
		int last = i+1;
//		clear();
		printf("TOP\n");
		printf("PROGRAM NAME\tCPU%%\tState\n");
		char * state_str;
		for ( i = 0 ; i < last ; i++ ) {
			printf("%s\t", array[i]->name);
			int percentage = array[i]->ticks;
			switch( array[i]->state ){
			  case RUNNING:
			    state_str = "Blocked";break;
			  case READY: 
			    state_str = "Ready";break;
			  case BLOCKED:
			    state_str = "Blocked"; break;
			}
			printf("%d.%d\t%s\n", ((int)percentage), sum, state_str); 
		}
		free(array);
	}
	return 0;
}

*/
