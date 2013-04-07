/*********************************************
kasm.h

************************************************/

#ifndef _kasm_
#define _kasm_


#include "defs.h"


unsigned int    _read_msw();

void            _lidt (IDTR *idtr);

void		_mascaraPIC1 (byte mascara);  /* Escribe mascara de PIC1 */
void		_mascaraPIC2 (byte mascara);  /* Escribe mascara de PIC2 */

void		_Cli(void);        /* Deshabilita interrupciones  */
void		_Sti(void);	 /* Habilita interrupciones  */

void		_int_00_hand();

void		_int_08_hand();      /* Timer tick */

void		_debug (void);
void 		_int_09_hand();
void		_int_80_hand(int selector, char character);
void		_int_0e_hand();
void 		_speaker(int, int);

char		_clock(int);

void		_setPaging(int);

int		_getStackSize();

void 		_out(int, int);

int		_in(int);

void 		_readSector(char *, int);

void		_writeSector(char *);

#endif
