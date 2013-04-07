#include "../include/kasm.h"
#include "../include/mempages.h"
#include "../include/libstd.h"
#include "../include/string.h"


extern PROCESS * currentProc;

SEMAPHORE memSem = DOWN;
//Tabla de directorios
unsigned int pageDirectory = 0x200000-5*0x1000;
//En las Tablas de Paginas soportamos 16MB (4MB Sistema y 12MB para el Usuario)  
 
 
//Tablda de datos de memoria malloqueada
struct data{
	unsigned int memory;
	unsigned int cantPages;
};
//Por default podemos allocar hasta 1024 veces simultaneamente.

void
setPages(){
  unsigned int i = 0;
  unsigned int k = 0;
  unsigned int j = 0;
  unsigned int h = 1;
  
  //seteo solo las primer 4 posiciones de la tabla de Directorios
  for(i = 0 ; i < 4; i++){
    ((unsigned int *)pageDirectory)[i] = pageDirectory + 0x1000 * h; //Las 4 paginas estan contiguas
    for( j = 0 ; j < 1024 ; j++ ){
    	((unsigned int *)(((unsigned int *)pageDirectory)[i]))[j] = k * 0x00001000;
	if( i < 2 ) //dejamos la primer pagina para el system (4MB)
		((unsigned int *)(((unsigned int *)pageDirectory)[i]))[j] += 0x223; //AVAIL EN 1 & 0 en user -> Es del sistema y NO PUEDO MALLOQUEAR
	else{
		if ( i > 2 ) {
			((unsigned int *)(((unsigned int *)pageDirectory)[i]))[j] += 0x027; //AVAIL EN 0 & 1 en user-> Es del usuario y PUEDO MALLOQUEAR
	/*	if ( i == 1 && j == 0) {
			printf("%d", ((unsigned int *)(((unsigned int *)pageDirectory)[i]))[j]);
		}*/
		} else {
			((unsigned int *)(((unsigned int *)pageDirectory)[i]))[j] += 0x027;
		}
	}
	k++;
    }
    ((unsigned int *)pageDirectory)[i] += 0x037;
    h++;
  }
  
  _setPaging(pageDirectory);


  return;
}

void * lastPage = (void *)(2 * 1024 * 0x00001000);

void *
getPage() {
	void * ret = lastPage;
	lastPage += 0x1000;
	return ret;
}

static void *
malloc2(int size, Header * p) {

	//Quizas haya que deshabilitar interrupciones.
	if ( p->next == NULL ) {
		//No hay lugar disponible
		if ( p->status == 0 ) {
			//printf("POR ACA?\n");
			return NULL;
		}
		if ( (int)p + sizeof(Header) + size >= (int)currentProc->lastHeapPage + 0x1000 ) {
			if ( currentProc->heapPages[currentProc->maxHeapPages - 1] == NULL ) {
				int i;
				for ( i = 0 ; i < currentProc->maxHeapPages ; i++ ) {
					if ( currentProc->heapPages[i] == NULL ) {
						p->next = currentProc->heapPages[i] = getPage();
						break;
					}
				}
				p->next->status = 1;
				p->next->next = NULL;
				p->size = (int)currentProc->lastHeapPage + 0x1000 - ((int)p + sizeof(Header));
				currentProc->lastHeapPage = p->next;
				return malloc2(size, p->next);
			}
		} else {
			p->status = 0;
			p->next = (Header*)((int)p + sizeof(Header) + size);
			p->next->next = NULL;
			p->next->status = 1;
			return (void*)((int)p + sizeof(Header));
		}
	}
	if ( p->status == 1 ) {
		if ( size >= p->size ) {
			return malloc2(size, p->next);
		} else {
			p->status = 0;
			return (void*)((int)p + sizeof(Header));
		}
	}
	return malloc2(size, p->next);
}

void *
malloc(int size){
	return malloc2(size, (Header*)(currentProc->firstHeapPage));

}
 

void
free(void * p) {
	//printHeaders((Header*)(512*0x01000));
	//while ( memSem == UP ) {
	//	;
	//}
	//memSem = UP;
	//printf("ef\n");
	
	Header * aux, *start = (Header*)((int)p - sizeof(Header));
	aux = start;
	//printf("s of header %d\taux %d\taux->next %d\taux->size %d\n",sizeof(Header), aux,aux->next);	
	//printf("LIBERO: %d\n", aux);
	aux->status = 1;
	signed int sum = -sizeof(Header);

	while ( aux != NULL && aux->status == 1 ) {
	//	printf("AUX = %d\n", aux);
		//Si la siguiente queda en otra pagina
		//printf("lala");
	//	printf("en while: aux->size %d\taux->next %d\n",aux->size,aux->next);
		sum += (aux->size + sizeof(Header));
		aux = aux->next;
		/*if ( aux + aux->size + sizeof(Header) >= (Header*)( (int)aux - (int)aux % 0x1000 + 0x1000) - sizeof(Header) ) {
			break;			
		}*/
	}
	start->size= sum;
	/*if(aux == NULL){
		start->next = NULL;
	}else{
		start->next = (Header *)((int)start + sizeof(Header) + start->size);
	}*/
	start->next = aux;
	//printf("start %d\tstart->next %d\tstart->size %d\n",start,start->next,start->size);
	//printf("----------------------------------------\n");
	//printf("sf\n");
	//printHeaders((Header*)(512*0x01000));
	//while(1);
	//memSem = DOWN;
}

/*
void
free(unsigned int memDir){
	int finished = 0;
	int unsigned cantPages;
	int i_mD = 0;
	
	while( !finished && i_mD < 1024 ){
		if(memData[i_mD].memory == memDir){
			cantPages = memData[i_mD].cantPages;
			memData[i_mD].memory = 0;
			memData[i_mD].cantPages = 0;
			finished = 1;
		}
		i_mD++;
	}
	
	if(!finished)
		return;
		
	finished = 0;
	int i_pD = 0;
	int i_pT = 0;
	int stillNotFound = 1;
	unsigned int currentPageTable;
	
	while( i_pD < 4 && !finished ){
		i_pT = 0;
		currentPageTable = (((unsigned int *)pageDirectory)[i_pD]& 0xFFFFF000);
		while(i_pT < 1024 && !finished){
			if(stillNotFound){
				if( memDir == ( ((unsigned int *)currentPageTable)[i_pT] & 0xFFFFF000 ) ){
					((unsigned int *)currentPageTable)[i_pT] &= 0xFFFFFDFF; //coloco 0 en avail
					stillNotFound = 0;
				}
			//Si aun no se hallo la dir y no es la correinte, sigue
			}else{
				cantPages = cantPages - 1;
				if(cantPages == 0)
					finished = 1;
				else
					((unsigned int *)currentPageTable)[i_pT] &= 0xFFFFFDFF; //coloco 0 en avail
			}
			i_pT++;
		}	  
		i_pD++;
	}
	
	return;
}
*/

/* Coloca en 0 a la pagina page
 */
void
clearPage(unsigned int page){
	int i = 0;
	for(;i<1024;i++)
		((unsigned int *)(page))[i] = 0x00;
	return;
}

void *
calloc(int cant, int size){
	void * p = malloc(cant*size);
	int i = 0;
	while ( i < cant*size ) {
		*(char*)(p + i) = 0;
		i++;
	}
	return p;		
}


/* Funcion para el usuario
 */
/*
unsigned int
malloc_user(char * name, int size){
  	int i = 0;
	int iAvlble = 2000;
	for(;i<1024;i++){
	  if(iAvlble == 2000 && userData[i].progName[0] == 0)
	  	iAvlble = i;  
	  else if(userData[i].progName[0] != 0 && !strcmp(userData[i].progName,name)){
	  	printf("Ese nombre ya esta ocupado.\n");
		return 0;
	  }  
	}
	void * dir = malloc(size);
	if(dir == 0){
		printf("Fallo de Memoria.\n");
		return 0;
	}
	userData[iAvlble].memory = dir;
	*/
	/*strcpy*/
/*
	i = 0;
	while(name[i]!=0 && i < 199){
		userData[iAvlble].progName[i] = name[i];
		i++;
	}
	userData[iAvlble].progName[i] = 0;
	return dir;
}
*/
/* Funcion para el usuario
*//*
unsigned int
calloc_user(char * name, int size){
  int i = 0;
  int iAvlble = 2000;
  for(;i<1024;i++){
    if(iAvlble == 2000 && userData[i].progName[0] == 0)
      iAvlble = i;  
    else if(userData[i].progName[0] != 0 && !strcmp(userData[i].progName,name)){
      printf("Ese nombre ya esta ocupado.\n");
      return 0;
    }  
  }
  unsigned int dir = calloc(size);
  if(dir == 0){
    printf("Fallo de Memoria.\n");
    return 0;
  }
  userData[iAvlble].memory = dir;
  */
  /*strcpy*//*
  i = 0;
  while(name[i]!=0 && i < 199){
    userData[iAvlble].progName[i] = name[i];
    i++;
  }
  userData[iAvlble].progName[i] = 0;
  return dir;
}
*/
/* Funcion para el usuario
 *//*
void
free_user(char * name){
  int i = 0;
  int j;
  for(;i<1024;i++){
    if(userData[i].progName != 0 && !strcmp(userData[i].progName,name)){
      free(userData[i].memory);
      for(j = 0 ; j < 200 ; j++)
	  userData[i].progName[j] = 0;
      userData[i].memory = 0;
      printf("Memoria desalojada con exito.\n");
      return;
    }  
  }
  printf("Nombre de programa invalido.\n");
  return;
}

*/
/*
char *
 getProgName(unsigned int mem){
 	int finished = 0;
	int i = 0;
	char * name = 0;
 	while(!finished && i<1024){
		if( userData[i].memory == mem ){
			name = userData[i].progName;
			finished = 1;
		}
		i++;
	}
	
	return name;
 }
*/
/* Funcion para el usuario. 
 * Imprime en la salida estandar la memoria con los mallocs hechos hasta el momento y el stack.
 *//*
 void
 listMemory(){
	int i;
	int memOcupada = 0; //en Bytes
	printf("\n--Direcciones de memoria reservadas : \n( Name (Dir) Cant de memoria ocupada en Bytes)\n\n");
	for(i = 0 ; i < 1024 ; i++)
		if(memData[i].memory != 0){
		  	printf("\t%s  (%d) %d\n",getProgName(memData[i].memory),memData[i].memory,memData[i].cantPages * 4 *1024);
			memOcupada += 4*1024*memData[i].cantPages; 
			}
	printf("\n--Cantidad de memoria libre : %d Bytes\n\n",(12*1024*1024 - memOcupada)); //El usuario cuenta con 12MB
	printf("Tamano del stack: %i\n", _getStackSize());
	return;
 }
 
 */
