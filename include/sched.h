#ifndef _sched_
#define _sched_

#include "defs.h"

void createProcSched(PROCESS * newidle_t);

/*Introduce al scheduler la tarea task*/
void insertProcess(PROCESS * proc);
	
/*Remueve un proceso*/
int removeProcess(PROCESS *);

/*Remueve un proceso por su pid*/
int removeProcessByPid(int);

//Funcion que almacena el ESP actual
void SaveESP (int ESP);

//Funcion que obtiene el ESP de idle para switchear entre tareas.
void* GetTemporaryESP (void);

//Funcion que devuelve el PROCESS* siguiente a ejecutar
PROCESS* GetNextProcess(void);

//Funcion Scheduler
PROCESS * GetNextTask();

//Funcion que devuelve el ESP del proceso actual.
int LoadESP(PROCESS* proc);

//Funcion a donde van a parar las funciones que terminan.
void Cleaner(void);

void resetCurrentProc();

//Recibe un entero que indica el estado de bloqueo del programa actual.
void block(int);

void unblock(int, TTY *);

int ktop(int, char**);

int printAllPids(int, char**);

#endif
