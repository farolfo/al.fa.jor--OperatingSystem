/**********************
 circularBuffer.h
**********************/

#ifndef _circularBuffer_
#define _circularBuffer_

#define BUFFER_SIZE 1024
#define ES_AR 0
#define EN_US 1

typedef struct circularBuffer {
  int vector[BUFFER_SIZE];
  int size;
  int next;
  int last;
  int iterator;
  int start;
} circularBuffer;

void putInBuffer(int, circularBuffer *, int, int);

int getFromBuffer(circularBuffer *);

void initialize(circularBuffer *);

int peekLast(circularBuffer *);

int hasNext(circularBuffer * cb);

int next(circularBuffer * cb);

void reset(circularBuffer * cb);

void getStringFromBegining(circularBuffer * cb, char *);

void putCharInBuffer(int character, circularBuffer * cb);

int isEmpty(circularBuffer * cb);

void removeLast(circularBuffer * cb);

void changeLanguage(int);

#endif 
