/*
 * Archivo de manejor de formato
*/

#include "../include/colormgmt.h"
//#include "../include/libstd.h"
#include "../include/string.h"
#include "../include/defs.h"

extern PROCESS * currentProc;

int parseCodeColor(char *);



/* Cambia el formato por el code del nuevo formato.
 */

void
changeFormat(char newFormat) {
 	currentProc->tty->format = newFormat;
}


/*Modificia el color del background para los sigueintes colores:
  -Black
  -Green
  -Red
  -Cyan
  -Brown
  -Blue
  -Magenta
  -White
  
  En caso de pasar mal el string con el color se retorna 0.*/

int
backgroundColor(char * str){
  char code = parseCodeColor(str);
  if(code == -1)
     return 0;
  currentProc->tty->format &= 0x0F;
  code *= 0x10; /* backroung color es 0xXY, siendo X =CODE y Y = code de colro de letra que venia*/
  currentProc->tty->format += code;
  return 1;
}


/*Modificia el color del letra para los sigueintes colores:
  -Black
  -Green
  -Red
  -Cyan
  -Brown
  -Blue
  -Magenta
  -White
  
  En caso de pasar mal el string con el color se retorna 0.*/
int
letterColor(char * str){
  char code = parseCodeColor(str);
  if(code == -1)
     return 0;
  currentProc->tty->format &= 0xF0;
  currentProc->tty->format += code;
  return 1;
}


/* Dado un string valido (black|green|red|cyan|brown|blue|magenta|white), sin importar mayusculas ni minusculas, 
  retorna el code correspondiente a dicho color:
  -Black	0
  -Green	2
  -Red		4
  -Cyan		3
  -Brown	6
  -Blue		1
  -Magenta	5
  -White	7
  
  En caso de ser un color invalido, retorna -1.
 */
int
parseCodeColor(char * str_const){
  if(str_const == 0 || str_const[0]=='\0')
    return -1;
  
  char str[10] = {0,0,0,0,0,0,0,0,0,0};
  int i = 0;
  while(str_const[i] != 0){
    str[i] = toupper(str_const[i]);
    i++;
  }
  
  int code;
  int length = strlen(str);
  int error = 0;
  switch(length){
    case 3: /*Red*/
      if(strcmp(str,"RED")==0)
	code = 0x04;
      else
	error = 1;
      break;
    case 4: /*Cyan Blue*/
      if(strcmp(str,"BLUE")==0)
	  code = 0x01;
      else if(strcmp(str,"CYAN")==0)
	    code = 0x03;
	else
	    error = 1;
      break;
    case 5: /*Black Green Brown White*/
      if(strcmp(str,"BLACK")==0)
	  code = 0x00;
      else 
	if(strcmp(str,"GREEN")==0)
	    code = 0x02;
	else 
	  if(strcmp(str,"BROWN")==0)
	      code = 0x06;
	  else 
	      if(strcmp(str,"WHITE")==0)
		  code = 0x07;
	      else
		  error = 1;
      break;
    case 7: /*Magenta*/
      if(strcmp(str,"MAGENTA")==0)
	  code = 0x05;
      else
	  error = 1;
      break;
    default:
      error = 1;
  }
  if(error == 1)
    return -1;
  
  return code;
}
