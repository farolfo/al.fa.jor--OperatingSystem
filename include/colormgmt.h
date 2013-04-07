/**********************
 colormgmt.h
**********************/

#ifndef _colormgmt_
#define _colormgmt_

void changeFormat(char);

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
int backgroundColor(char * str);

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
int letterColor(char * str);

#endif
