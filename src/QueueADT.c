#include "../include/libstd.h"
#include "../include/mempages.h"
#include "../include/QueueADT.h"
#include "../include/defs.h"
#include "../include/kernel.h"

QueueADT
newQueue(){
	QueueADT queue = (QueueADT) kmalloc(sizeof(QueueCDT));
	if( queue == NULL ) {
		printf("Malloc failed2.");
		return NULL;
	}
	queue->first = NULL;
	queue->last = NULL;
	queue->size = 0;
	
	return queue;
}

int
queueSize(QueueADT q){
	return q->size;
}


/**
 *	0 -> COLA CON DATOS
 *	1 -> COLA VACIA
 * */
int
queueIsEmpty(QueueADT queue){
	return (queue->size) ? 0 : 1 ;
}


/**
 *	Retorna NULL en caso de no haber elementos a desencolar.
 */
void *
dequeue(QueueADT queue){
	if( queue->first == NULL )
		return NULL;

	void * ans = queue->first->data;
	QNode * eus = queue->first;


	queue->first = queue->first->next; 
	free(eus);	
	queue->size --;
	if ( queue->size == 1 ) {
		queue->last = queue->first;
	}
	return ans;
}


/**
 *	Retorna -1 en caso de error al encolar. 
 */
int
enqueue(QueueADT queue, void * data){
	QNode * newNode = (QNode *) kmalloc(sizeof(QNode));
	if( newNode == NULL ){
		printf("Malloc failed.");
		return -1;
	}
	
	newNode->data = data;
	newNode->next = NULL;

	if( queue->size == 0 ){
		queue->first = newNode;
	}else{
		queue->last->next = newNode;
	}
	queue->last = newNode;
	queue->size++;
	return 0;
}

/**
 *  Remueve el nodo de la cola
 *  Retorna -1 si no lo hallo o hubo problemas
  */
int removeFromQueue(QueueADT queue, void * data){
	if( queue == NULL ){
		printf("ERROR EN QUEUE-REMOVE : queue vale NULL\n");
		return -1;
	}
	
	QNode * current = queue->first;
	QNode * last = NULL;
	while( current != NULL ){
		if(current->data == data){
			if(last == NULL){/*Es el primer elemento el que hay que remover*/
				queue->first = current->next;
			}
			else{
				last->next = current->next;
			}
			if(current->next == NULL){
				queue->last = last;
			}
			queue->size--;
			return 1;
		}
		last = current;
		current = current->next;
	}
	return -1;
}

/*
 * -------  Funciones de Iterator ----   */

int
queueIteratorSize() {
	return sizeof(QIterator);
}

void
queueIteratorReset(QueueADT queue, QIterator * it) {
	it->node = queue->first;
}

void *
queueNext(QIterator * it) {
	if ( it->node == NULL ) {
		return NULL;
	}
	QNode * aux = it->node;
	it->node = it->node->next;
	return aux->data;
}



/**
 *	Funcion de testing
 *	Descomentar el main sieguiente y correr solo QueueADT.c
 */
/*
int 
main(void){
	QueueADT * queue = newQueue();

	printf("Cola vacia : %s\n",(queueIsEmpty(queue))? "OK" : "FAILED");
	
	int f = 1;
	int g = 2;
	int h = 3;

	enqueue(queue,&f);
	enqueue(queue,&g);
	enqueue(queue,&h);
	
	printf("Size : %s\n",(queue->size == 3)?"OK":"FAILED");
	printf("Cola con datos (isEmpty test) : %s\n", (!queueIsEmpty(queue)) ? "OK" : "FAILED" );

	int *a = (int*)dequeue(queue);//1
	int *b = (int*)dequeue(queue);//2
	int *c = (int*)dequeue(queue);//3

	printf("Enqueue y dequeue : %s\n", (*a==1 && *b==2 && *c==3 && queueIsEmpty(queue))? "OK" : "FAILED" );



	return 0;
}
*/
