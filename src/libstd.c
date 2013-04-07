#include "../include/kasm.h"
#include "../include/defs.h"
#include "../include/kc.h"
#include "../include/libstd.h"
#include <stdarg.h>
#include "../include/kernel.h"
#include "../include/circularBuffer.h"


/*Bibliografia:                                             */
/*http://heim.ifi.uio.no/~inf3150/grupper/1/pcspeaker.html  */
 
/*Imprime con el formato de printf de ANSI C. Exepto que un string q termine en % no lo verifica
  y solo imprime con formato:
    d,i - entero decimal con signo.
    s   - string.
    
  */
  
extern int tick;
int seed = 1;
extern circularBuffer ioBuffer;

int
printf(const char * string, ...) {
  va_list ap;
  int i = 0;
  int ival;
  char * sval;
  int cant_dig;
  int ival_aux;
  
  if ( string == 0 ) {
    return -1;
  }
  
  va_start(ap,string);
  
  while ( (*(string+i)) != '\0' ) {
    if((*(string+i))!='%'){
       putchar(*(string+i));
    }else{
       switch(*(string+i+1)){
	 case 'd': case 'i':
	   ival = va_arg(ap, int);
	   
	   if(ival < 0){
	     putchar('-');
	     ival = ival *-1;
	   }
	   
	   cant_dig = getCantDig(ival); 
	   
	   while( cant_dig != 0 ){
	     ival_aux = ival;
	     ival_aux = ival_aux / pow((int)10,cant_dig-1);
	     putchar( ( ival_aux % 10 ) + '0' );
	     cant_dig--;
	   } 
	   
	   break;
	 case 's':
	   sval = va_arg(ap, char *);
	   bprintf(sval);
	   break;
	 case '%':
	   putchar('%');
	   break;
       }
       i++;
    }
    i++;
  }
  va_end(ap);
  return i;
}


/*Retorna la cantidad de digitos de num*/
int
getCantDig(int num){
  int cant=1;
  num = num/10;

  while(num != 0){
    cant++;
    num = num/10;
  }
  
  return cant;/*for(i=0 ; i<count; i++){
	  while ( (res = getFromBuffer()) == NULL ) {
	    printf("asds");
	  }
	  printf("salio del getformbfer con : %d",res);
	  ((int *)buffer)[i] = res;*/
}


/*Funcion de potencia con exponente positiva.*/
int
pow(int base, int exp){
  int i;
  int result = 1;
  if(exp == 0)
    return 1;
  
  for( i = exp ; i != 0 ; i--)
    result *= base;
  
  return result;
}

/*Imprime un string */
int 
bprintf(const char * string) {
  int i=0;
  
  if((*string) == 0 )
    return -1;

  while((*(string+i)) != 0 ){/*for(i=0 ; i<count; i++){
	  while ( (res = getFromBuffer()) == NULL ) {
	    printf("asds");
	  }
	  printf("salio del getformbfer con : %d",res);
	  ((int *)buffer)[i] = res;*/
    putchar(*(string+i));
    i++;
  } 
  
  return i;
}

int 
putc(int c, int fd){
  if ( __write(fd, &c, 1) == -1 ) {
    return -1;
  }
  return (int)c;
}


int 
putchar(int c) {
  return putc(c,SCREEN); 
}

int 
getchar(){
  return getc(KEYBOARD);
}

int 
getc(int fd) {
  int c;
  __read(fd, &c, 1);
  return c;
}

int scanint(int *pint, char*message, int* resp) {
	int final;
	char result[30];
	int offset = 0;

	while (isDigit(message[offset])) {
		result[offset] = message[offset];
		offset++;
	}
	*resp = (!isDigit(message[offset])) ? 0 : 1;
	result[offset] = 0;
	final = stratoi(result);
	*(pint) = final;
	return offset;

}


int scanstring(char* pchar, char*message, int* resp) {

	int i = 0;
	while (message[i] != '\0' && message[i] != ' ') {
		pchar[i] = message[i];
		i++;
	}
	pchar[i] = '\0';
	*resp = 1;
	return i;
}

int scanf(const char * string,...)
 {
	va_list ap;
	int scanfresult = 0;
	int c, i, bufpos = 0, percentflag = 0, endFlag = 0;
	char stringRead[150];
	char * character;
	
	va_start(ap, string);
	
	while ((c = getchar()) != '\n') {
	  if (c != '\0') {
	      stringRead[i++] = c;
	  }
	}

	i = 0;
	while (string[i] != '\0' && !endFlag) {
		if (string[i] == '%' && !percentflag) {
			i++;
			switch (string[i]) {
			case 'd': case 'i':
				bufpos += scanint(va_arg(ap,int*),stringRead+bufpos,&scanfresult);
				break;
			case 's':
				bufpos += scanstring(va_arg(ap,char*),stringRead+bufpos,&scanfresult);
				break;
			case 'c':
				character = va_arg(ap,char*);
				*(character) = stringRead[bufpos];
				bufpos++;
				break;
			case '%':
				percentflag = 1;
				break;
			default:
				putchar(string[i]);
				printf("Invalid argument type error. Float not supported or anything else");
			}
			i++;
		} else {
			if (string[i] != stringRead[bufpos]) {
				endFlag = 1;
			} else {
				i++;
				bufpos++;
				percentflag = 0;
			}
		}
	}
	va_end(ap);
	return scanfresult;
}

int stratoi(char* str) {

	int c = 0; int resp = 0; int module = 1;
	while (str[c] != '\0') {
		if (str[c] == '-') {
			module = -1;
			c++;
		}
		if (str[c] <  '0' || str[c]  > '9' )
			return -1;
	
	   resp *= 10;
	   resp += (str[c] - '0');
	   c++;
	}
	resp *= module;
	return resp;

}

int isDigit(int a) {
	return (a >= '0' && a <= '9');
}

void 
memcpy(char* a, char* b, int len)
{
	int i;
	for(i = 0; i < len; i++)
	{
		a[i] = b[i];
	}
	return;
}

int
random() {
	//int aux = ((double)n) / 60.0;
	//printf("RAND > %d\n",n);
	//printf("PAPA: ---0.\n");
	//printf("ads %d, %d", , (double)n);
	//printf("%d", ((double)n) / 60);
	//printf("%d\n", n);
	int n = tick * 7 + ( 97 * seed );
	seed ++;
	return n % 100;
}
