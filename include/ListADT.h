/****************
 * ListADT.h
 ***************/

#ifndef _ListADT_
#define _ListADT_

typedef struct ListADT ListADT;

typedef struct Iterator Iterator;

/*
 * Devuelve una lista dados una función de comparación y una de liberación
 */
ListADT * newListADT(int (*fun)(void *, void*), void (*)(void *));

/*
 * Si la lista está vacía retorna 1. Si no, retorna 0.
 */
int listIsEmpty(ListADT *);

/*
 * Agrega un elemento a la lista
 */
int add(ListADT *, void *);

/*
 * DEPRECATED
 * Agrega un elemento a la lista si este no se encuentra todavía.
 */
int addNotRepeated(ListADT *, void *);

/*
 * Remueve un elemento de la lista y lo retorna
 */
void * removeFromList(ListADT *, void *);

/*
 * DEPRECATED
 * Copia una lista
 */
ListADT * copyList(ListADT *);

/*
 * Libera una lista y todos sus elementos
 */
void freeList(ListADT *);

/*
 * A diferencia de freeList, no libera los elementos que contiene. Sólo libera la lista y sus estructuras internas
 */
void shallowFreeList(ListADT *);

/*
 * Dado un iterador, obtiene el siguiente elemento, o NULL si no hay siguiente.
 */
void * listNext(ListADT *, Iterator *);

/*
 * Retorna el tamaño de la lista
 */
int getSize(ListADT *);

/*
 * Dado un arreglo, retorna una lista con los valores del arreglo.
 * Es necesario proporcionar una función de comparación y una de liberación.
 */
ListADT * arrayToList(void **, int (*) (void *, void *), void (*fr)(void *), int, int);

/*
 * Resetea el iterador dado, es decir, la proxima llamada a listNext con el iterador devolverá el primer elemento de la lista
 */
void listIteratorReset(ListADT *i, Iterator *);

/*
 * Si el elemento pertenece a la lista retorna 1. Si no, retorna 0
 */
int belongsToList(ListADT *, void *);

/*
 * Retorna el tamaño de la estructura Iterator, ya que es el usuario el encargado de alocar espacio para ella.
 */
int iteratorSize();

/*
 * Remueve de la lista el elemento i-ésimo
 */
void removeFromListI(ListADT *, int);

#endif
