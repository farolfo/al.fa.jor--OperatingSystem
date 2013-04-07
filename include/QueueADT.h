#ifndef _QueueADT_
#define _QueueADT_
typedef struct QNode{
	void * data;
	struct QNode * next; 
} QNode;

typedef struct QueueCDT{
	QNode * first;
	QNode * last;
	int size;
} QueueCDT;

typedef struct QIterator {
  QNode * node;
} QIterator;

typedef struct QueueCDT * QueueADT;

QueueADT newQueue();

/**
 *	0 -> COLA CON DATOS
 *	1 -> COLA VACIA
 * */
int queueIsEmpty(QueueADT queue);

/**
 *	Retorna NULL en caso de no haber elementos a desencolar.
 */
void * dequeue(QueueADT queue);

/**
 *	Retorna -1 en caso de error al encolar. 
 */
int enqueue(QueueADT queue, void * data);

/**
 *  Remueve el nodo de la cola
 *  Retorna -1 si no lo hallo.
  */
int removeFromQueue(QueueADT queue, void * data);

/* Funciones de Iterator */
int queueIteratorSize();

void queueIteratorReset(QueueADT queue, QIterator * it);

void * queueNext(QIterator * it);

int queueSize(QueueADT);
  
#endif
