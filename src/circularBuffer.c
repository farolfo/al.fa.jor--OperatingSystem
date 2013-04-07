#include "../include/circularBuffer.h"
#include "../include/libstd.h"
#include "../include/kasm.h"
#include "../include/sched.h"
#include "../include/defs.h"
#include "../include/kernel.h"

circularBuffer generalBuffer;
circularBuffer ioBuffer;

int language = ES_AR;
int accent = 0;
extern TTY * ttys[4];
extern int currentTTY;


void initialize(circularBuffer * cb) {
  (*cb).iterator = (*cb).last = BUFFER_SIZE - 1;
  cb->size = 0;
  cb->start = 0;
  cb->next = 0;
  int i;
  for ( i = 0 ; i < BUFFER_SIZE ; i++ ) {
	  cb->vector[i] = 0;
  }

  
}

int getKey(int scanCode, int shift) {
  scanCode &= 0x00FF;
  int ascii[][58] = { { -1, -1, '1', '2','3','4','5','6','7','8','9','0', '\'', 168, '\b', '\t', 'q', 'w', 'e', 
		  'r', 't', 'y', 'u', 'i', 'o', 'p', -2, '+', '\n', -1 , 'a', 's', 'd', 'f', 'g', 'h', 'j',
		    'k', 'l', 164, '{', '#', -1, '}', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-',
		    -1, -1, -1, ' '},
		    { -1, -1, '!', '"','#','$','%','&','/','(',')','=', '?', 173, '\b', '\t', 'Q', 'W', 'E', 
		  'R', 'T', 'Y', 'U', 'I', 'O', 'P', -3, '*', '\n', -1 , 'A', 'S', 'D', 'F', 'G', 'H', 'J',
		    'K', 'L', 165, '[', '#', -1, ']', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_',
		    -1, -1, -1, ' ' },
		    { -1, -1, '1', '2','3','4','5','6','7','8','9','0', '\'', 168, '\b', '\t', 'q', 'w', 'e', 
		  'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', -1 , 'a', 's', 'd', 'f', 'g', 'h', 'j',
		    'k', 'l', ';', '\'', '#', -1, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
		    -1, -1, -1, ' '},
		    { -1, -1, '!', '@','#','$','%','^','&','*','(',')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 
		  'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', -1 , 'A', 'S', 'D', 'F', 'G', 'H', 'J',
		    'K', 'L', ':', '"', '|', -1, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
		    -1, -1, -1, ' '}  };
  
 
  if ( scanCode >= 58 ) {
    return -1;
  }
  return ascii[language*2+shift][scanCode];
}

void putInBuffer(int scanCode, circularBuffer * cb, int shift, int capsLock) {
//printf(" put in buffer - Entrando1...\n");
  if ( (*cb).size == BUFFER_SIZE ) { 
//	printf("pib salgo\n");
    return;
  }
//printf("pib-por aca paso\n");
  int nextEntry = (*cb).last + 1;
  int modifier = (shift && (1- capsLock))||(capsLock && (1-shift));
  int letter = getKey(scanCode,0);
  if ( letter != '\n' ) {
     if ( (*cb).size == BUFFER_SIZE - 1 ) { 
//	printf("pib salgo1\n");
	return;
     }
  }
  if ( letter == -1 ) {
    return;
  }
  if ( (letter >= 'a' && letter <= 'z') || letter == 164 ) {
    if ( accent != 0 ) {
      switch(accent) {
	case -2:
	  switch(letter) {
	    case 'a':
	      if ( modifier ) {
		letter = getKey(scanCode, modifier);;
	      } else {
		letter = 160;
	      }break;
	    case 'e':
	      if ( modifier ) {
		letter = getKey(scanCode, modifier);;
	      } else {
		letter = 130;
	      }break;
	    case 'i':
	      if ( modifier ) {
		letter = getKey(scanCode, modifier);;
	      } else {
		letter = 161;
	      }break;
	    case 'o':
	      if ( modifier ) {
		letter = getKey(scanCode, modifier);;
	      } else {
		letter = 162;
	      }break;
	    case 'u':
	      if ( modifier ) {
		letter = getKey(scanCode, modifier);;
	      } else {
		letter = 163;
	      }break;
	    default:
	      letter = getKey(scanCode, modifier); break;
	  }break;
	case -3:
	    if ( letter == 'u' ) {
	      if ( modifier ) {
		letter = getKey(scanCode, modifier);;
	      } else {
		letter = 129;
	      }
	    } else {
	      letter = getKey(scanCode, modifier);
	    }break;
      }
      accent = 0;
    } else {
	letter = getKey(scanCode, modifier);
    }
  } else if ( letter == -2 ) {
      accent = -2 - (shift?1:0);
      return;
  } else {
    letter = getKey(scanCode, shift); 
  }
  if ( (*cb).last == BUFFER_SIZE - 1 ) {
    nextEntry = 0;
  }
  (*cb).vector[nextEntry] = letter;
  ((*cb).size)++;
  (*cb).last = nextEntry;
 // printf("Desbloqueando...\n");
  unblock(BLOCK_READ, ttys[currentTTY]);
}

void putCharInBuffer(int character, circularBuffer * cb) {
  int nextEntry = (*cb).last + 1;
  if ( (*cb).last == BUFFER_SIZE - 1 ) {
    nextEntry = 0;
  }
  (*cb).vector[nextEntry] = character;
  ((*cb).size)++;
  (*cb).last = nextEntry;
  //printf("Desbloqueando...\n");
  unblock(BLOCK_READ, ttys[currentTTY]);
  return;
}

/*EN EL BUFFER NULL ES EL VALOR DE VACIO.*/
int 
getFromBuffer(circularBuffer * cb) {
  if ( (*cb).vector[(*cb).next] == NULL ) {
    return NULL;
  }
  char ret = (*cb).vector[(*cb).next];
  (*cb).vector[(*cb).next] = NULL;
  ((*cb).size)--;
  if ( (*cb).next == BUFFER_SIZE - 1 ) {
    (*cb).next = 0;
  }
  else {
    (*cb).next++;
  }
  return (int)ret;
}

int peekLast(circularBuffer * cb) {
  if ( (*cb).last == BUFFER_SIZE - 1 ) {
    return (*cb).vector[0];
  }
  return (*cb).vector[(*cb).last];
}

int hasNext(circularBuffer * cb) {
  return (*cb).last != (*cb).iterator;
}

int next(circularBuffer * cb) {
  while ( 1 ) {	
	  if ( !hasNext(cb) ) {
		  //printf("Bloqueando...\n");
		  block(BLOCK_READ);
		  asm("int $0x08");
	  }
	  if ( hasNext(cb) ) {
	 
	  	if ( (*cb).iterator == BUFFER_SIZE - 1 ) {
	        	(*cb).iterator = 0;
	        	return (*cb).vector[0];
	        }
	        (*cb).iterator++;
	        return (*cb).vector[(*cb).iterator];
	  }
  }
}

void reset(circularBuffer * cb) {
  int newValue = (*cb).last + 1;
  if ( newValue == BUFFER_SIZE ) {
    newValue = 0;
  }
  (*cb).start = newValue;
}

void getStringFromBegining(circularBuffer * cb, char * string) {

  int i = (*cb).start;
  int count = 0;
  while ( i < (*cb).last ) {
    if ( (*cb).vector[i] == '\b' ) {
     (*cb).vector[i] = NULL;
      if ( count != 0 ) {
	count --;
      } 
    }
    else {
      string[count] = (*cb).vector[i];
      (*cb).vector[i] = NULL;
      count++;
    }
    i++;
    
  }
  (*cb).size -= (count + 1);
  string[count] = '\0';
  (*cb).start = (*cb).last + 1;
}

int isEmpty(circularBuffer * cb) {
 return (*cb).vector[(*cb).next] == NULL;
}

void removeLast(circularBuffer * cb) {
  (*cb).vector[(*cb).last] = NULL;
  if ( (*cb).last == 0 ) {
    (*cb).last = BUFFER_SIZE - 1;
  } else {
    (*cb).last--;
  }
}

void changeLanguage(int newLang) {
  language = newLang;
  return;
}
