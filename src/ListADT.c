#include "../include/libstd.h"
#include "../include/mempages.h"
#include "../include/kernel.h"

//#include <string.h>

typedef struct Node {
	void * data;
	struct Node * next;
} Node;

typedef struct Iterator {
	Node * node;
} Iterator;

typedef struct ListADT {
	Node * first;       
	int size;
	int (*cmp) (void *, void *);
	void (*fr) (void *);
//	Node * iterator;
//	Node * itPrev;
} ListADT;

static void
freeNode(ListADT * list, Node * node) {
	list->fr(node->data);
	free(node);
}

ListADT *
newListADT(int (*fun)(void *, void *), void (*fr)(void *) ) { /*fun deuvelve cero en caso de ser iguales*/
	ListADT * list = (ListADT *) kmalloc(sizeof(ListADT));
	if ( list == NULL ) {
		//perror("Malloc failed");
		return NULL; /*nuuuuull*/
	}
	list->first = NULL;
	list->size = 0;
	list->cmp = fun;
//	list->iterator = NULL;
//	list->itPrev = NULL;
	list->fr = fr;
	return list;
}

int
listIsEmpty(ListADT * list) {
	return list->size == 0;
}

int
add(ListADT * list, void * data) {
	if ( list == NULL ) { 
		return -1; 
	}
	Node * node = (Node *) kmalloc(sizeof(Node));
	if ( node == NULL ) {
		//perror("Malloc failed");
		return -1;
	}
	node->data = data;
	node->next = list->first;
	list->first = node;
	list->size++;
	return 0;
}

int
addNotRepeated(ListADT * list, void * data) {

	if ( list->cmp == NULL ) {
		return -1;
	}

	if (  list == NULL ) {
		//perror("Invalid argument");
		return -1;
	}
	Node * node = (Node *) kmalloc(sizeof(Node));
	if ( node == NULL ) {
	//	perror("Malloc failed");
		return -1;
	}
	node->data = data;
	node->next = NULL;

	if ( list->size == 0 ) {
		list->first = node;
		(list->size)++;
		return 0;
	}
	Node * current = list->first;
	while ( current != NULL ) {
		if ( !list->cmp(current->data, data) ) {
			free(node);
			return 1;
		}
		current = current->next;
	}
/*	if ( list->iterator == list->first ) {
		list->iterator = node;
	}*/
	node->next = list->first;
	list->first = node;
	(list->size)++;
	return 0;
}

static Node *
removeNode(ListADT * list, Node * current, void * data, void ** ret) {
	if ( current == NULL ) {
		return NULL;
	}
	if ( !list->cmp(current->data, data) ) {
	/*	if ( list->iterator == current ) {
			list->iterator = current->next;
		}*/
		Node * next = current->next;
		*ret = current->data;
		list->size--;
		return next;
	}
	current->next = removeNode(list, current->next, data, ret);
	return current;
}

void *
removeFromList(ListADT * list, void * data) {
	if ( list == NULL ) {
		return NULL;
	}
	if ( list->cmp == NULL ) {
	//	perror("Remove not supported");
		return NULL;
	}
	void * ret;
	ret = NULL;
	list->first = removeNode( list, list->first, data, &ret );
	return ret;
}

ListADT *
copyList(ListADT * list) {
	ListADT * list2 = newListADT(list->cmp, list->fr);
	if ( list2 == NULL ) {
	//	perror("Malloc failed");
		return NULL;
	}
	Node * current = list->first;
	while ( current != NULL ) {
		add(list2, current->data);
		current = current->next;
	}
	return list2;
}



void
freeList(ListADT * list) {
	if( list == NULL ) {
		return;
	}

	Node * next;
	while(list->first !=  NULL){
		next = list->first->next;
		freeNode(list, list->first);
		list->first = next;	
	}
	free(list);
}

void
shallowFreeList(ListADT * list) {
	if ( list == NULL ) {
		return;
	}
	Node * aux;
	while ( list->first != NULL ) {
		aux = list->first->next;
		free(list->first);
		list->first = aux;
	}
	free(list);
}

void *
listNext(ListADT * list, Iterator * it) {
	if ( it->node == NULL ) {
		return NULL;
	}
	Node * aux = it->node;
	it->node = it->node->next;
	return aux->data;
/*	if ( list->iterator == NULL ) {
		if ( list->itPrev == (Node *) -1 ) {
			return NULL;
		}
		list->iterator = list->first;
		list->itPrev = NULL;
		return (list->iterator == NULL) ? NULL : list->iterator->data;
	}
	list->itPrev = list->iterator;
	list->iterator = list->iterator->next;
	if ( list->iterator == NULL ) {
		list->itPrev = (Node *) -1;
		return NULL;
	}
	return list->iterator->data;*/
	/*
	if ( list->iterator == NULL ) {
		return NULL;
	}
	if ( list->itPrev == NULL ) {

	}
	list->itPrev = list->itPrev->next;
	Node * aux = list->iterator;
	list->iterator = list->iterator->next;
	return aux->data;
	*/
}

void
listIteratorReset(ListADT * list, Iterator * it) {
	it->node = list->first;
}
/*
void
listIteratorRemove(ListADT * list) {
	list->itPrev->next = list->iterator->next;
	free(list->iterator);
	list->iterator = list->itPrev;
}
*/
int
getSize(ListADT * list) {
	return list->size;
}

ListADT *
arrayToList(void ** array, int (*cmp)(void *, void *), void (*fr)(void *), int size, int elemSize) {
	ListADT * list = newListADT(cmp, fr);
	if ( list == NULL ) {
		return NULL;
	}
	int i;
	void * elem;
	for ( i = 0 ; i < size ; i++ ) {
		elem = kmalloc(elemSize);
		memcpy(elem, array[i], elemSize); 
		add(list, elem);
	}
	return list;
}

int
belongsToList(ListADT * list, void * elem) {
	void * aux;
	Iterator * it = kmalloc(sizeof(Iterator));
	listIteratorReset(list, it);
	while ( ( aux = listNext(list, it) ) != NULL ) {
		if ( !list->cmp(aux, elem) ) {
			return 1;
		}
	}
	free(it);
	return 0;
} 

int
iteratorSize() {
	return sizeof(Iterator);
}

void
removeFromListI(ListADT * list, int index) {
	Iterator * it = kmalloc(iteratorSize());
	listIteratorReset(list, it);
	Node * current = list->first, * prev = NULL;
	int i = 0;
	while ( current != NULL ) {
		if ( i == index ) {
			list->size--;
			if ( prev == NULL ) {
				list->first = current->next;
				free(current);
				free(it);
				return;
			}
			prev->next = current->next;
			free(current);
			free(it);
			return;
			
		}
		prev = current;
		current = current->next;
		i++;
	}
	free(it);
}
