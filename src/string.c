#include "../include/libstd.h"
#include "../include/mempages.h"
#include "../include/kernel.h"



void strspcat(const char* str , char* r1 , char* r2)

{
  while (*str != ' ' && *str)
  { 
       *r1 = *str;
        r1++;
	str++;
  }
  if ( *str ) {
    str++;
  }
  while ( *str )
  {   
        *r2 = *str;
        r2++;
	str++;
  }
  *r1 = '\0';
  *r2 = '\0';
  return;
  
}

int strlen(const char *s)
{
    int size = 0;
    while(*s++) 
      size++;
    return size;

}

int strcmp(const char *s1, const char *s2)
{
      while((*s1 && *s2) && (*s1 == *s2))
        s1++,s2++;

      return *s1 - *s2;

}

/*Funcion que convierte a mayuscula un caracter*/
char toupper(char s)
{
        if(('a' <= s) && (s <= 'z'))
             s = 'A' + (s - 'a');
	return s;
}

/*Funcion que convierte a minuscula un caracter*/
char tolower(char s)
{

        if(('A' <= s) && (s <= 'Z'))
            s = 'a' + (s - 'A');
		return s;
}

int
endsWith(char * str, char c){
	if(str == 0)
		return 0;
	int len = strlen(str);
	return ( str[len-1] == c )? 1 : 0 ;
}

void strcpy(char * s1, char * s2) {
	while ( *s2 ) {
		*s1 = *s2;
		s1++;
		s2++;
	}
	*s1 = '\0';
}


/* concaten alos dos strings, str + char c1. el resultado se guarda en str y str debe tener lugar */
void
charcat(char * str, char c){
	int end = strlen(str);
	str[end] = c;
	str[end+1] = '\0';
	return;
}

void
strcat(char * dest, char * src){
	int i = 0;
	int size = strlen(src);
	for(; i < size; i++){
		charcat( dest, src[i]);
	}
	return;
}

char **
split(char * str2, char divider, int * cant){
	int i;
	int len = strlen(str2);
	char * str = kmalloc(len + 1);
	strcpy(str, str2);
	char ** ans = NULL;
	char ** aux;
	int start = 0;
	int size = 0;
	for ( i = 0 ; str[i] != '\0' ; i++ ) {
		if ( size % 10 == 0 ) {
			aux = ans;
			ans = kmalloc(size + 10*sizeof(char*));
			if ( aux != NULL ) {
				memcpy((char*)ans, (char*)aux, size*sizeof(char*));
				free(aux);
			}
		}
		if ( str[i] == divider ) {
			str[i] = '\0';
			ans[size] = kmalloc(i - start + 1);
			strcpy(ans[size], str + start);
			start = i + 1;
			size++;
		}
	}
	
	if ( size % 10 == 0 ) {
		aux = ans;
		ans = kmalloc(size + 10*sizeof(char*));
		if ( aux != NULL ) {
			memcpy((char*)ans, (char*)aux, size);
			free(aux);
		}
	}
	ans[size] = kmalloc(i - start + 1);
	strcpy(ans[size], str + start);
	start = i + 1;
	size++;
	free(str);

	*cant = size;
//	int ww = 0;
//	for ( ww = 0 ; ww < size ; ww++ ) {
//		printf("AUXasddas = %s\n", ans[ww]);
//	}
	return ans;
}
