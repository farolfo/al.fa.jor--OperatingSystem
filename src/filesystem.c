#include "../include/filesystem.h"
#include "../include/kernel.h"
#include "../include/mempages.h"
#include "../include/string.h"
#include "../include/defs.h"
#include "../include/libstd.h"
#include "../include/kasm.h"


#define MAX(a, b) (((a) > (b)) ? (a) : (b))

void * start;

typedef char * BLOCK; 

//ENTRY * cwd;
//ENTRY * pcwd;
ENTRY * root;
BLOCK blocks;
INODE * inodes;
PROCESS * currentProc;

static int getFreeInode(char * name, int prev, int flag, int);
int open(char * path);

struct FD_ROW{
	int pid;
	BLOCK file; /*puntero al bloque en memoria*/
	int offset;
	int currentBlock; 
	INODE * inode;
	int inodeNumber;
};

struct FD_ROW openFiles[50];
char freeInodes[2043];

void
printInode( int inodeNumber){
  	ATTR i = inodes[inodeNumber].attributes;
	printf("INODO --- size = %d, alive = %d, prev_version = %d, creation time = %d/%d/%d:%d:%d:%d,  name = %s, parent= %d", i.size, i.alive, i.prev_version, i.creation_time.day, i.creation_time.month, i.creation_time.year, i.creation_time.hours, i.creation_time.minutes, i.creation_time.seconds, i.name, i.parentDir);
	printf("BLOCKS: ");
	int x = 0;
	while ( x < BLOCKS_PER_FILE && inodes[inodeNumber].blocks[x] != -1) {
		printf(": %d", inodes[inodeNumber].blocks[x]); 
		x++;
	}
	printf("\n");
}

int read(int, void *, int);

void
printDir( int inodeNumber ) {
  	INODE  i = inodes[inodeNumber];
	BLOCK b = blocks + 512*i.blocks[0];
	int x;
//	int fd = open("/");
//	ENTRY aux;
	for ( x = 0 ;  ; x++ ) {
		if (  !strcmp(((ENTRY*) (b+sizeof(ENTRY)*x))->name, "") ) {
			return;
		}
//		read(fd, &aux, sizeof(ENTRY));
//		printf("ENTRY: %d --%s--\n", aux.inodeNumber, aux.name );
	}
}





static void loadINode(INODE * inode, int inodeNumber) {
	useSector(5+inodeNumber, (char*)inode, READ, 1);
	return;
}

/* En base a un inodeNumber, me carga a memoria dicho inodo y em devuelve el puntero a dicho inodo */
static INODE * 
getInode( int inodeNumber) {
	INODE * inode = kmalloc(sizeof(INODE));
	if ( inode == NULL ) {
		printf("FALLO EL MALLOC DE GETINODE\n");
		return NULL;
	}
	loadINode(inode, inodeNumber);
	return inode;
}



static void loadBlock(BLOCK block, int blockNumber) {
	useSector(2048+blockNumber*7, block, READ, 7);
	return;
}


/* En base a un blockNumber, me lo carga en memoria y me deuvelve el putnerp*/
static char *
getBlock( int blockNumber ){
	BLOCK block = kmalloc(7*512);
	if (block == NULL ) {
		printf("Fallo el malloc de getBlock\n");
		return 0;
	}
	loadBlock(block, blockNumber);
	return block;
} 

static void
loadBitMap( char * bitMap){
	useSector(1, bitMap, READ, 4);
	return;
}

static void
setBitMap( char * bitMap){
	useSector(1, bitMap, WRITE, 4);
	return;
}

static void
setBlock(BLOCK block, int blockNumber) {
	useSector(2048+blockNumber*7, block, WRITE, 7);
}

static int
getFreeBlock(){
	char bitMap[4*512];
	loadBitMap(bitMap);
	int j = 0;
	int i = 0;
	for( j = 0; j < 512*4 && bitMap[j] == 0x00 ; j++ ) {
	}	
	if( j == 512 * 4 ){
		return -1;	
	}
	int k = 0;

	for ( i = 0 ; i < sizeof(char) * 8 ; i++ ) {
		if ( ((bitMap[j] << i) & 0x80) == 0x80 ) {
			int aux = 0x1;
			for ( k = 0 ; k < sizeof(char)*8 - i - 1 ; k++ ) {
				aux <<= 1;
			}
			aux = ~aux;
			bitMap[j] &= aux;
			setBitMap(bitMap);
			return j * sizeof(char) * 8 + i;
		}
	}	
	return 0xCABA110;
}

static int
setFreeBlock(int blockNumber){
	if( blockNumber <= 0 || blockNumber > 2047 ){
		return -1;
	}
	
	char bitMap[4*512];
	loadBitMap(bitMap);
	
	int indexToModify = blockNumber / 8;
	int indexOfIndexToModify = blockNumber % 8;
	
	char newEntry;
	switch( indexOfIndexToModify ){
		case 0: newEntry = 0x80; break;
		case 1: newEntry = 0x40; break;
		case 2: newEntry = 0x20; break;
		case 3: newEntry = 0x10; break;
		case 4: newEntry = 0x08; break;
		case 5: newEntry = 0x04; break;
		case 6: newEntry = 0x02; break;
		case 7: newEntry = 0x01; break;
	}
	
	bitMap[indexToModify] |= newEntry;
	
	setBitMap(bitMap);
	
	return 0;
}


static void
setInode(INODE * inode, int inodeNumber){
	useSector(5+inodeNumber, (char*)inode, WRITE, 1);
	return;

}

int
close(int fd) {
	if ( fd < 0 || fd > 50 ) {
		return -1;
	}
	if ( currentProc->pid != openFiles[fd].pid ) {
		return -1;
	}
	setInode(openFiles[fd].inode, openFiles[fd].inodeNumber);
	free(openFiles[fd].file);
	openFiles[fd].file = NULL;
	free(openFiles[fd].inode);
	return 0;
}


static int
getFreeInode(char * name, int prev, int flag, int parent) {
	int i = 0;
	for ( i = 0 ; i < 2043 ; i++ ) {
		if ( freeInodes[i] ) {
			break;
		}
	}
	if ( i == 2043 ) {
		return -1;
	}
	INODE * inode = kmalloc(sizeof(INODE));
	strcpy(inode->attributes.name, name);
	int seconds = _clock(0);
	inode->attributes.creation_time.seconds = ((seconds&0xF0) >> 4) * 10 + (seconds&0x0F); 
	int minutes = _clock(2);
	inode->attributes.creation_time.minutes = ((minutes&0xF0) >> 4) * 10 + (minutes&0x0F); 
	int hours = _clock(4);
	inode->attributes.creation_time.hours = ((hours&0xF0) >> 4) * 10 + (hours&0x0F); 
	int day = _clock(7);
	inode->attributes.creation_time.day = ((day&0xF0) >> 4) * 10 + (day&0x0F);
	int month = _clock(8);
	inode->attributes.creation_time.month = ((month&0xF0) >> 4) * 10 + (month&0x0F); 
	int year = _clock(9);
	inode->attributes.creation_time.year = ((year&0xF0) >> 4) * 10 + (year&0x0F);
	inode->attributes.prev_version = prev;
	inode->attributes.size = 0;
	inode->attributes.alive = 1;
	inode->attributes.free = 0;
	inode->attributes.parentDir = parent;
	int j = 0;
	if ( flag ) {
		inode->blocks[0] = getFreeBlock();
		j++;
	}
	inode->blocks[j] = -1;
	setInode(inode, i);
	free(inode);
	freeInodes[i] = 0;
	return i;
}

/* pseudo open que en base un inode number me lo carga en la tabla para hacer raed y open y baialr la conga*/
static int
popen(int inodeNumber){
	INODE * inode = getInode(inodeNumber);
	BLOCK block = getBlock(inode->blocks[0]);
	if( block == NULL ) {
		printf("ERROR  AL ABRIR EL FILE\n");
		return -1;
	}
	/* Inicializaicon en tabla */
	int i;
	for( i=0; i<50; i++){
		if (openFiles[i].file == NULL)
			break;
	}
	if( i == 50 ){
		printf("Tabla llena\n");
		return -1;
	}
	
	openFiles[i].pid = currentProc->pid;
	openFiles[i].file = block;
	openFiles[i].offset = 0;
	openFiles[i].currentBlock = 0;
	openFiles[i].inode = inode;
	openFiles[i].inodeNumber = inodeNumber;

	return i;
}

/* Parsea el path y me mallokea un Entry con el name y inode Number correspondiente. Si no existia, dir sera null */
static void
setEntry( char * path, ENTRY * dir, int flag) {
//printf("SETTING ENTRY\n");
	char ** aux;
	if ( !strcmp(path, "/") ) {
		memcpy((char*)dir, (char*)root, sizeof(ENTRY));
		return;	
	}
	if ( !strcmp(path, "") ) {
		ENTRY nothing;
		strcpy(nothing.name, "");
		memcpy( (char*) dir, (char*) &nothing, sizeof(ENTRY) );
		return;
	}
	int size;
	aux = split(path, '/', &size);
	ENTRY * current = kmalloc(sizeof(ENTRY));
	int i;
	int start = 1;
	if ( !strcmp(aux[0], "") ) {
		memcpy( (char*)current, (char*)root, sizeof(ENTRY));
	} else {
		if ( !strcmp(aux[0], ".") ) {
			memcpy( (char*)current, (char*)&(currentProc->tty->cwd), sizeof(ENTRY));
		} else if ( !strcmp(aux[0], "..") ) {
			memcpy( (char*)current, (char*)&(currentProc->tty->pcwd), sizeof(ENTRY));
		} else {
		/*	memcpy( (char*)current, (char*)&(currentProc->tty->cwd), sizeof(ENTRY));
			int xx;
			for ( xx = strlen(path) ; xx >= 0 ; xx-- ) {
				path[xx+2] = path[xx];
			}
			path[0] = '.';
			path[1] = '/';
			start = 0;*/
			ENTRY nothing;
			strcpy(nothing.name, "");
			memcpy( (char*) dir, (char*) &nothing, sizeof(ENTRY) );
			free(current);
			for ( i = 0 ; i < size ; i++ ) {
				free(aux[i]);
			}
			free(aux);
			return;
		}
	}
//printf("seteado el start\n");
	int exists = 1;
	i = start;
	while( i < size && exists) {
//printf("I = %d\n", i);
		int j, fd;
		fd = popen(current->inodeNumber);
		ENTRY currentEntry;
		for ( j = 0 ; read(fd, &currentEntry, sizeof(ENTRY)) == sizeof(ENTRY) ; j++ ) {
//printf("ENTRY {%s, %d}\n", currentEntry.name, currentEntry.inodeNumber);
			if ( !strcmp(currentEntry.name, "") ) {
				exists = 0;
				break;
			}
//printf("GETTING INODE %d\n", currentEntry.inodeNumber);
			INODE * auxInode = getInode(currentEntry.inodeNumber);
//printf("GET INODE\n");
			if ( auxInode->attributes.alive || flag ) {
				if ( !strcmp(currentEntry.name, aux[i]) ) {
					*current = currentEntry;
					free(auxInode);
					break;
				}
			}
			free(auxInode);
			
		}  
		close(fd);
		i++;
	}
//printf("POR AQUI %d\n", size);
	for ( i = 0 ; i < size ; i++ ) {
		free(aux[i]);
	}
//printf("LIBERADO AUX[i -> AUX = %d]\n", aux);
	free(aux);
//printf("LIBERADO AUX\n");
	if ( !exists ) {
		//printf("CPIANDO\n");
		strcpy(current->name, "");
		//printf("COPIADO\n");
	}
	memcpy((char*)dir, (char*)current, sizeof(ENTRY));
	free(current);
//printf("TERMINO SET ENTRY\n");
}


void 
init_fs() {
	char block[512] = {0};
	useSector(0, block, READ,1);

	if ( *((int*)block) == 0x12345678 ) {
	  	//printSomewhere(300, "POR ACA");
		root = kmalloc(sizeof(ENTRY));
		strcpy(root->name, "/");
		root->inodeNumber = 0;
		root->link = 0;
		int i;
		for ( i = 5 ; i < 2048 ; i++ ) {
			INODE * inodeAux = getInode(i-5);
			freeInodes[i-5] = inodeAux->attributes.free;
			free(inodeAux);
		}

		return;
	}
	*((int*)block) = 0x12345678;
	ENTRY rootEntry;
	strcpy(rootEntry.name , "/");
	rootEntry.inodeNumber = 0;
	rootEntry.link = 0;
	memcpy( block + sizeof(int), (char*)&rootEntry, sizeof(ENTRY));
	useSector(0, block, WRITE, 1);
	
	/*sete mapa d bits*/
	int i;
	char mapBlock[4*512];
	for ( i = 1 ; i < 4*512 ; i++ ) {
		mapBlock[i] = 0xFF;
	}
	mapBlock[0] = 0x7F;
	setBitMap(mapBlock);
	INODE inode;
	inode.attributes.free = 1;
	inode.blocks[0] = -1;
	for ( i = 5 ; i < 2048 ; i++ ) {
		if ( i == 5 ) {
			INODE rootInode;
			rootInode.blocks[0] = 0;
			rootInode.blocks[1] = -1;
			rootInode.attributes.prev_version = -1;
			rootInode.attributes.parentDir = 0;	
			int seconds = _clock(0);
			rootInode.attributes.creation_time.seconds = ((seconds&0xF0) >> 4) * 10 + (seconds&0x0F); 
			int minutes = _clock(2);
			rootInode.attributes.creation_time.minutes = ((minutes&0xF0) >> 4) * 10 + (minutes&0x0F); 
			int hours = _clock(4);
			rootInode.attributes.creation_time.hours = ((hours&0xF0) >> 4) * 10 + (hours&0x0F); 
			int day = _clock(7);
			rootInode.attributes.creation_time.day = ((day&0xF0) >> 4) * 10 + (day&0x0F);
			int month = _clock(8);
			rootInode.attributes.creation_time.month = ((month&0xF0) >> 4) * 10 + (month&0x0F); 
			int year = _clock(9);
			rootInode.attributes.creation_time.year = ((year&0xF0) >> 4) * 10 + (year&0x0F); 
			strcpy(rootInode.attributes.name , "/");	
			rootInode.attributes.size = 3 * sizeof(ENTRY);
			rootInode.attributes.free = 0;
			rootInode.attributes.alive = 1;
			char buffer[512] = {0};
			memcpy(buffer, (char*)&rootInode, 512);
			useSector(5, buffer, WRITE, 1);
		} else {
			char buffer[512] ={0};
			memcpy(buffer, (char*)&inode, 512);
			useSector(i, buffer, WRITE, 1);
		}
	}
	for ( i = 1 ; i < 2043 ; i++ ) {
	  	freeInodes[i] = 1;
	}
	freeInodes[0] = 0;

	root = kmalloc(sizeof(ENTRY));
	strcpy(root->name, "/");
	root->inodeNumber = 0;
	root->link = 0;
	
	char buffer[7*512];
	strcpy(((ENTRY*) buffer)[0].name, ".");
	strcpy(((ENTRY*) buffer)[1].name, "..");
	((ENTRY*)buffer)[0].inodeNumber = ((ENTRY*)buffer)[1].inodeNumber = 0;
	strcpy(((ENTRY*)blocks)[2].name, "");
	//Seteo el bloque 0, que es el 2048
	useSector(2048, buffer, WRITE, 7);
	
	return;
}





int
read(int fd, void * buffer, int cant){
//   printf("LEO\n");
	if( fd < 0 || fd > 50 ){
		return -1;
	}
	if ( openFiles[fd].pid != currentProc->pid ) {
		return -1;
	}
	int os = openFiles[fd].offset % (7*512);
	int i = 0;
	while ( 1 ) {
		while ( os < (7*512) && openFiles[fd].offset + i < openFiles[fd].inode->attributes.size && i < cant )  {
			((char*)buffer)[i] = openFiles[fd].file[os] ;
			os++;
			i++;
		}
//printf("PASE WHILE\n");
		if ( openFiles[fd].offset + i == openFiles[fd].inode->attributes.size ) {
			openFiles[fd].offset += i;
//printf("TERMINE4\n");
			return i;
		}
		if ( i == cant ) {
			openFiles[fd].offset += i;
//printf("TERMINE3\n");
			return i;
		}
		os = 0;
		if ( openFiles[fd].currentBlock == BLOCKS_PER_FILE - 1 ) {
			openFiles[fd].offset += i;
//printf("TERMINE2\n");
			return i;
		}
		openFiles[fd].currentBlock++;
		free(openFiles[fd].file);
		openFiles[fd].file = getBlock(openFiles[fd].inode->blocks[openFiles[fd].currentBlock]);
		if ( openFiles[fd].file==NULL){
			printf("Mal el bloque\n");return i;
		}
//printf("OBTUVE BLOQUE\n");
	}
	openFiles[fd].offset += i;
//printf("TERMINE\n");
	return i;


}

static int
cloneBlock(int n){

	BLOCK block = getBlock(n);
	int newBlockN = getFreeBlock();
	BLOCK newBlock = getBlock(newBlockN);
	memcpy(newBlock, block, 7*512);
	setBlock(newBlock, newBlockN);
	
	free(block);
	free(newBlock);
	return newBlockN;
}


// copia el inodo y lo carga en el disco, se le pasa el inodeNumber en el aprametro(el del current) y se deuvelve alli el numb del nuevo
static INODE *
cloneInode(INODE * inode, int * inodeNumber){
	int inodeAnsNumb = getFreeInode(inode->attributes.name, inode->attributes.prev_version, 0, inode->attributes.parentDir);
	//printf("FREE %d\n", inodeAnsNumb);
	if ( inodeAnsNumb < 0 ) {
		printf("No quedan inodos disponibles\n");
		return NULL;
	}
	INODE * inodeAns = getInode(inodeAnsNumb);
	//printf("INODE %d\n", inodeAns);
	inodeAns->attributes.size = inode->attributes.size;
	inodeAns->attributes.parentDir = inode->attributes.parentDir;
	int i = 0;
	while( inode->blocks[i] != -1 && i < BLOCKS_PER_FILE){
		inodeAns->blocks[i] = inode->blocks[i];
		i++;
	}
	inodeAns->blocks[i] = -1;
	*inodeNumber = inodeAnsNumb;
	setInode(inodeAns, inodeAnsNumb);
	return inodeAns;
}

//HAY QUE VER EL CASO EN QUE SE ESCRIBE ALGO QUE PISA PARTE DEL CONTENIDO (PORQUE OFFSET NO 
//F_END. EN ESE CASO, EL SIZE NO ES EL SIZE ULTIMO + I.
int
write(int fd, int offset, void * buffer, int cant) {
	if ( fd < 0 || fd > 50 ) {
		return -1;
	}
	if ( openFiles[fd].pid != currentProc->pid ) {
		return -1;
	}
	if ( offset == F_END ) {
		offset = openFiles[fd].inode->attributes.size;
	} else {
		if ( offset < 0 ) {
			return -1;
		}
	}
	int aux = offset / (7*512);
	BLOCK block;
	int newInodeNumb = openFiles[fd].inodeNumber;
	INODE * newInode =  cloneInode(openFiles[fd].inode, &newInodeNumb);
	int blockNumb;
	blockNumb = cloneBlock(openFiles[fd].inode->blocks[aux]);
	openFiles[fd].inode->blocks[aux] = blockNumb;
	newInode->attributes.prev_version = openFiles[fd].inode->attributes.prev_version;
	openFiles[fd].inode->attributes.prev_version = newInodeNumb;
	if ( aux != openFiles[fd].currentBlock ) {
		block = getBlock(openFiles[fd].inode->blocks[aux]);
	} else{
		block = openFiles[fd].file;
	}
	int os = offset % (7*512);
	int i = 0;// j;
	while ( 1 ) {
		while ( i < cant && os < 7*512 )  {
			block[os] = ((char*)buffer)[i];
			os++;
			i++;
		}
		if ( i == cant ) {
			break;
		}
		os = 0;
		setBlock(block, blockNumb);
		aux++;
		if ( aux  == BLOCKS_PER_FILE - 1 ) {
			break;
		} 
		if ( openFiles[fd].inode->blocks[aux] != -1 ) {
			blockNumb = cloneBlock(openFiles[fd].inode->blocks[aux]);
			openFiles[fd].inode->blocks[aux] = blockNumb;
		} else {
			blockNumb = getFreeBlock();
			//for ( j = 0 ; j < BLOCKS_PER_FILE ; j++ ) {
				//if ( openFiles[fd].inode->blocks[j] == -1 ) {
					//openFiles[fd].inode->blocks[j] = blockNumb;
					//if ( j < BLOCKS_PER_FILE - 1 ) { //Me parece que se puede comentar...si entro en el else, estoy asegurando que hay al menos 1 con -1.
						openFiles[fd].inode->blocks[aux] = blockNumb;
						openFiles[fd].inode->blocks[aux+1] = -1;
					//}
					//break;
				//}
			//}//Creo que esta parte no va...se valida mas arriba
/*			if ( j == BLOCKS_PER_FILE ) {
				openFiles[fd].inode->attributes.size = MAX(openFiles[fd].inode->attributes.size, offset + i); //+= i;
				setInode(openFiles[fd].inode, openFiles[fd].inodeNumber );
				printf("SIZE = %d\n", openFiles[fd].inode->attributes.size );
				return i;
			}*/
		}
		if ( block != openFiles[fd].file ) {
			free(block);
		}
		block = getBlock(blockNumb);
	}
//	printf("START2\n");
	setBlock(block, blockNumb);
	openFiles[fd].inode->attributes.size = MAX(openFiles[fd].inode->attributes.size, offset + cant);
	setInode(openFiles[fd].inode,  openFiles[fd].inodeNumber);
	setInode(newInode, newInodeNumb);
	free(newInode);
	if ( block != openFiles[fd].file ) {
		free(block);
	}
//	printf("STOP2\n");
	return cant;
}

int
writeNS(int fd, int offset, void * buffer, int cant) {
	int ret = write(fd, offset, buffer, cant);
	INODE * prev = getInode(openFiles[fd].inode->attributes.prev_version);
//printf("INODE RETRIEVED1\n");
	int prevVersion = prev->attributes.prev_version;
	freeInodes[openFiles[fd].inode->attributes.prev_version] = 1;
	prev->attributes.free = 1;
	setInode(prev, openFiles[fd].inode->attributes.prev_version);
//printf("INODE SET1\n");
	openFiles[fd].inode->attributes.prev_version = prevVersion;
	setInode(openFiles[fd].inode, openFiles[fd].inodeNumber);
	free(prev);
//printf("INODE SET2\n");
	return ret;
}


static int
create(char * path, int parentDir ) {
//printf("create\n");
	int i;
	for ( i = 0 ; i < 2043 ; i++ ) {
		if ( freeInodes[i] == 1 ) {
			break;
		}
	}
	if ( i == 2043 ) {
		printf("No hay espacio\n");
		return -1;
	}
	int len = strlen(path);
	for ( i = len ; path[i] != '/' && i > 0; i-- ) {

	}
	if ( i == len ) {
		printf("No hay nombre de archivo\n");
		return -1;
	}
	if ( len - i >= 30 ) {
		printf("El nombre es muy largo %d\n", len-i);
		return -1;
	}
	char aux[30];
	strcpy(aux, path+i+1);
	char auxChar = *(path+i+1);
	*(path+i+1) = '\0';
	if ( strlen(path) > 1 ) {
		*(path+i) = '\0';
	}
	int fd = open(path);
//printf("ABRI %s en %d de tamano %d\n", openFiles[fd].inode->attributes.name,  openFiles[fd].inodeNumber,  openFiles[fd].inode->attributes.size);
	*(path+i) = '/';
	*(path+i+1) = auxChar;
	int count = 0;
	ENTRY aux_entry;
	for ( i = 0 ; read(fd, &aux_entry, sizeof(ENTRY)) == sizeof(ENTRY) ; i++ ) {
//printf("{%s, %d}\n", aux_entry.name, aux_entry.inodeNumber);
		if ( !strcmp(aux_entry.name, "") ) {
			strcpy(aux_entry.name, aux);
			ENTRY aux2_entry;
			strcpy(aux2_entry.name, "");
//printf("Malloqueando\n");
			char * buffer = kmalloc(2*sizeof(ENTRY));
//printf("Listo\n");
			int ret = aux_entry.inodeNumber = getFreeInode(aux, -1, 1, parentDir); //Devuelve un numero de inodo vacio ya seteado en disco.
			aux_entry.link = 0;
			memcpy(buffer, (char*)(&aux_entry), sizeof(ENTRY));
			memcpy(buffer + sizeof(ENTRY), (char*)(&aux2_entry), sizeof(ENTRY));
			write(fd, count*sizeof(ENTRY), buffer, 2*sizeof(ENTRY));
			free(buffer);
			close(fd);
			return ret;
		}
		count++;
	}
//printf("Saliendo de create\n");
	close(fd);
	return -1;	

}

int
mkdir(char * path) {
//printf("MKDIR\n");
	ENTRY dir;
	int i;
	//printf("PATH %s\n", path);
	setEntry(path, &dir,0);
//printf("ENTRY SET\n");
	if ( !strcmp(dir.name, "") ) {
		int len = strlen(path);
		for ( i = len ; path[i] != '/' && i>=0 ; i-- ) {
	
		}
		if ( i == len ) {
			printf("No hay nombre de archivo\n");
			return -1;
		}
		if ( len - i >= 30 ) {
			printf("El nombre es muy largo -- %d, %d\n", len, i );
			return -1;
		}
		char aux[30];
		strcpy(aux, path+i+1);
		char auxChar = *(path+i+1);
		*(path+i+1) = '\0';
		ENTRY parent;
		setEntry(path, &parent,0);
		*(path+i+1) = auxChar;
//printf("PATH %s\n", path);
		create(path, parent.inodeNumber);
//printf("SUPERE CREATE\n");
		int fd = open(path);
		ENTRY entry;
		ENTRY entry2;
		ENTRY entry3;
		strcpy(entry.name, ".");
		entry.inodeNumber = openFiles[fd].inodeNumber;
		strcpy(entry2.name, "..");
		entry2.inodeNumber = openFiles[fd].inode->attributes.parentDir;
		strcpy(entry3.name, "");

		char * buffer = kmalloc(3*sizeof(ENTRY));
		memcpy(buffer, (char*)(&entry), sizeof(ENTRY));
		memcpy(buffer + sizeof(ENTRY), (char*)(&entry2), sizeof(ENTRY));
		memcpy(buffer + 2*sizeof(ENTRY), (char*)(&entry3), sizeof(ENTRY));
		writeNS(fd, 0, buffer, 3*sizeof(ENTRY));
//printf("ESCRIBI\n");
		free(buffer);
		close(fd);
		return 0;
	}
	return -1;
}

int 
open(char * path){
//printf("entro open\n");
	ENTRY dir;
	int i1;
	int len = strlen(path);
	for ( i1 = len ; path[i1] != '/' && i1 >= 0 ; i1-- ) {
	}
	if ( len - i1 >= 30 ) {
		printf("El nombre es muy largo %d -- %d---%d\n", len -i1, len, i1);
		return -1;
	}
	char aux[30];
	strcpy(aux, path+i1+1);
	char auxChar = *(path+i1+1);
	*(path+i1+1) = '\0';
	if ( strlen(path) > 1 ) {
		*(path+i1) = '\0';
	}
	//printf("PATH = <%s>\n", path);
	ENTRY parent;
	setEntry(path, &parent,0);
	//printf("FATHER = %s\n", parent.name);
	if ( !strcmp(parent.name, "") ) {
		printf("SALE OPEN MAL\n ");
		return -1;
	}
	*(path+i1) = '/';
	*(path+i1+1) = auxChar;
	setEntry( path, &dir,0);
	INODE * inode = NULL;
	BLOCK block;
	int dir_inodeNumber;
	if( !strcmp( dir.name, "") ){
//printf("Llamando a create\n");
		dir_inodeNumber = create(path, -1);
		if ( dir_inodeNumber == -1 ) {
			return -1;
		}
		dir.inodeNumber = dir_inodeNumber;
	}
	inode = getInode(dir.inodeNumber);
	block = getBlock(inode->blocks[0]);
	if( block == NULL ) {
		printf("ERROR  AL ABRIR EL FILE \n");
		return -1;
	}
	/* Inicializaicon en tabla */
	int i;
	for( i=0; i<50; i++){
		if (openFiles[i].file == NULL)
			break;
	}
	if( i == 50 ){
		printf("Tabla de file descriptors llena\n");
		return -1;
	}
	
	openFiles[i].pid = currentProc->pid;
	openFiles[i].file = block;
	openFiles[i].offset = 0;
	openFiles[i].currentBlock = 0;
	openFiles[i].inode = inode;
	openFiles[i].inodeNumber = dir.inodeNumber;
//printf("salgo open\n");
	return i;
}


static int
isDir(int n) {
	INODE * inode = getInode(n);
	if ( inode != NULL ) {
		free(inode);
		return inode->attributes.parentDir != -1;
	} 
	free(inode);
	return 0;
}

void
ls(char * path) {
	ENTRY aux;
	if( path[0] == 0 ){
		memcpy( (char *)&aux, (char *)&(currentProc->tty->cwd), sizeof(ENTRY));	
	}else{
		setEntry(path, &aux,0);
		if( aux.name[0] == 0 || !isDir(aux.inodeNumber) ){
			printf("No existe directorio\n");
			return;
		}
	}
	int i, fd = popen(aux.inodeNumber);
	ENTRY aux_entry;
	printf("\tName\t\t\t\t|\tSize\t\t\t\t|\tState\n");
	printf("-----------------------------------------------------\n");
	for ( i = 0 ; read(fd, &aux_entry, sizeof(ENTRY)) == sizeof(ENTRY) ; i++ ) {
		if ( !strcmp(aux_entry.name, "") ) {
			close(fd);
			return;
		}
		INODE * auxInode = getInode(aux_entry.inodeNumber);
		if ( aux_entry.name[0] != '.' ) {
		  printf("\t%s\t\t\t\t|\t%d\t\t\t\t|\t%s\n", aux_entry.name, auxInode->attributes.size,
			       ((auxInode->attributes.alive)?"ALIVE":"DEAD" ));
		}
		free(auxInode);
	}
	close(fd);	
}


void
cd(char * path) {
	ENTRY aux;
	setEntry(path, &aux,0);
	if ( aux.name[0] != '\0' ) {
		if ( isDir(aux.inodeNumber) ) {
			ENTRY parentEntry;
			int len = strlen(path);
			int i;
			for ( i = len ; path[i] != '/' && i > 0; i-- ) {

			}
			
			char aux2[30];
			strcpy(aux2, path+i+1);
			char auxChar = *(path+i+1);
			*(path+i+1) = '\0';
			if ( strlen(path) > 1 ) {
				*(path+i) = '\0';
			}
			setEntry(path, &parentEntry,0);
			*(path+i) = '/';
			*(path+i+1) = auxChar;

			memcpy((char*)&(currentProc->tty->cwd),(char*)&aux, sizeof(ENTRY));
			memcpy((char*)&(currentProc->tty->pcwd),(char*)&parentEntry, sizeof(ENTRY));
			return;
		}
	} 
	printf("No existe el directorio\n");
}


int
readPage(int fd) {
	char buffer[101];
	int size = read(fd, buffer, 100);
	buffer[size] = '\0';
	printf(buffer);
	return size; 
}

int
readAt(char * path, int offset) {
	int fd = open(path);
	if( fd == -1 ){
		printf("El arhcivo o directorio no existe.\n");
		return -1;
	}
	
	char buffer[100];
	lseek(fd, offset);
	int j;
	if(  (j = read( fd, &buffer, 100)) ){
		int i =0;
		for(; i < j; i++)
		  	putchar(buffer[i]);
		putchar('\n');
		close(fd);
		return 1;
	}
	printf("Nada para leer.\n");
	return 0;
}

int
addToDir(ENTRY dirEntry, ENTRY newEntry){
	int fd = popen(dirEntry.inodeNumber);
	if ( fd < 0 ) {
		return -1;
	}
	int count = 0,i;
	ENTRY aux_entry;
	for ( i = 0 ; read(fd, &aux_entry, sizeof(ENTRY)) == sizeof(ENTRY) ; i++ ) {
		if ( !strcmp(aux_entry.name, "") ) {
			ENTRY aux2_entry;
			strcpy(aux2_entry.name, "");
			char * buffer = kmalloc(2*sizeof(ENTRY));
			memcpy(buffer, (char*)(&newEntry), sizeof(ENTRY));
			memcpy(buffer + sizeof(ENTRY), (char*)(&aux2_entry), sizeof(ENTRY));
			write(fd, count*sizeof(ENTRY), buffer, 2*sizeof(ENTRY));
			free(buffer);
			close(fd);
			return 0;
		}

		INODE * auxInode = getInode(aux_entry.inodeNumber);
		if ( auxInode->attributes.alive ) {
			if ( !strcmp(aux_entry.name, newEntry.name) ) {
				close(fd);
				printf("Nombre repetido\n");
				return -1;
			}
		}
		free(auxInode);
				count++;
	}
	close(fd);
	return -1;
}

int
cp(char * from, char * to) {
	ENTRY fromEntry;
	ENTRY toEntry;
	setEntry(from, &fromEntry,0);
	if ( fromEntry.name[0] == '\0' ) {
		printf("No existe directorio\n");
		return -1;
	}
	INODE * inodeFrom = getInode(fromEntry.inodeNumber);
	INODE * inodeTo;
	int len = strlen(to);
	int i;
	for ( i = len ; to[i] != '/' && i > 0; i-- ) {
	}
	char aux[30];
	strcpy(aux, to+i+1);
	char auxChar = *(to+i+1);
	*(to+i+1) = '\0';
	if ( strlen(to) > 1 ) {
		*(to+i) = '\0';
	}
	setEntry(to, &toEntry,0); 
	*(to+i) = '/';
	*(to+i+1) = auxChar;
	if ( toEntry.name[0] == '\0' ) {
		printf("No existe el directorio destino\n");
		free(inodeFrom);
		return -1;
	}
	int newNumb = fromEntry.inodeNumber;
	inodeTo = cloneInode(inodeFrom, &newNumb);
	int w;
	strcpy(inodeTo->attributes.name, aux);
	for ( w = 0 ; w < BLOCKS_PER_FILE && inodeFrom->blocks[w] != -1 ; w++ ) {
		inodeTo->blocks[w] = cloneBlock(inodeFrom->blocks[w]);
	}
	setInode(inodeTo, newNumb);
	ENTRY entryToAdd;
	entryToAdd.inodeNumber = newNumb;
	strcpy( entryToAdd.name, aux);
	addToDir( toEntry, entryToAdd);
	free(inodeFrom);
	free(inodeTo);
	return 0;
}

/* si se llamo a cat desde el shell con un solo argumento, se llama a printFile */
void
printFile(char * path){
	int fd = open(path);
	if(fd == -1){
		return;
	}

	char buffer[300];
	int i, sizeRead;
	while( 1 ){
		sizeRead = read(fd, buffer, 300);
		for( i = 0; i < sizeRead; i++){
			putchar(buffer[i]);
		}
		if( sizeRead != 300 ){
			break;
		}
	}
	putchar('\n');

	close(fd);
	return;
}

/*cat:
Concatena el archivo src en el archivo dest, y luego lo muestra por salida estandar.
Si alguno de los archivos no existe, lo crea.
Si no hay mas espacio, retorna -1. Sino 0.
Si el archivo dest esta vacio (size = 0), no se crea la snapshot de dest.*/
int
cat(char * dest, char * src){
  	int fdDest = open(dest);
 	if(fdDest < 0){
      		printf("El archivo no existe");
      		return -1;
 	 }
  	write(fdDest,F_END,src,strlen(src));
	close(fdDest);
	printFile(dest);

	return 0;
}

int
lseek(int fd, int offset){
	if ( fd < 0 || fd > 50 ) {
		return -1;
	}
	if ( openFiles[fd].pid != currentProc->pid ) {
		return -1;
	}

	openFiles[fd].offset = offset;

	return 0;
}

void
getNameFromPath(char * path, char * buff){
//printf("getname le llega <%s mide %d>",path,strlen(path));
	int len = strlen(path);
	int i;
	for ( i = len ; path[i] != '/' && i > 0; i-- ) {
//printf("=iteracion%d=",i);
	}
	if ( i == 0 && path[i] != '/') {
//printf("<salio por aca>");
		strcpy(buff, path);
		return;
	}
//printf("engetname <%s> tendira q ser dir",path+1+i);
	strcpy(buff, path+i+1);
//printf("engetname <%s> idem", buff);
	return;
}

/* Se que el arhcivo existe por lo menos una sola vez */
int
existsMoreThanOneDead( int fdCont, char * name){
    ENTRY aux;
    int i, conta = 0;
    for ( i = 0 ; read(fdCont, &aux, sizeof(ENTRY)) == sizeof(ENTRY) ; i++ ) {
        if( aux.name[0] == 0 ){
            return 0;
        }
        INODE * auxInode = getInode(aux.inodeNumber);
        //if ( auxInode->attributes.alive == 0 ) {
            if ( !strcmp(aux.name, name) ) {
                conta++;
                if( conta  == 2 ){
                    free(auxInode);
                    return 1;
                }
            }
        //}
        free(auxInode);
    }
    return 0;
}

int
remove(char * path) {
      /* Si el arhciv no existe, sale */
      ENTRY aux;
      setEntry(path, &aux,0);
      //printf("ENTRY SET\n");
      if ( aux.name[0] == '\0' ) {
          printf("No existe el archivo o directorio.\n");
            return -1;
      }
    if( ! strcmp( aux.name, "/") ){
        printf("No se puede borrar el directorio root.\n");    
        return -1;
    }
    if( ! strcmp( aux.name, ".") || !strcmp( aux.name, "..") ){
        printf("No puedes borrar este directorio desde esta locacion.\n");
        return -1;
    }
    
      /* Si ya hay un arhcivo muerto con es nombre , no se peude hacer rm */
      int len = strlen(path), i;
      for ( i = len ; path[i] != '/' && i > 0; i-- ) {
      }
      char auxChar = *(path+i+1);
      *(path+i+1) = '\0';
      if ( strlen(path) > 1 ) {
            *(path+i) = '\0';
      }
    /* Abro el directorio del padre */
    ENTRY cont;
    setEntry( path, &cont,0);
    if( cont.name[0] == 0 ){
        printf("Ocurrio un error inesperado.\n");
        return -1;
    }
      int fdCont = popen( cont.inodeNumber);
    if( fdCont == -1 ){
        printf("Ocurrio un error inesperado2.\n");
        return -1;
    }
    if( existsMoreThanOneDead( fdCont, aux.name) ){
        printf("No se puede tener dos archivos con este nombre muertos. Debes hacer un forcerm de este archivo.\n");
        close(fdCont);
        return -1;
    }
    close(fdCont);
      *(path+i) = '/';
     *(path+i+1) = auxChar;
    
    /* Seteo el flag de alive en 0 */
    INODE * inode = getInode(aux.inodeNumber);
//printf("inode retrieved\n");
    inode->attributes.alive = 0;
    setInode(inode, aux.inodeNumber);
//printf("Inode set\n");
    free(inode);
    return 0;    
} 
int
mv(char * from, char * to) {
	ENTRY aux;
	setEntry(from, &aux,0);
	if ( aux.name[0] == '\0' ) {
		printf("No existe fuente\n");
		return -1;
	}
	int inodeNumber = aux.inodeNumber;
	int len1 = strlen(from), i1;
	for ( i1 = len1 ; from[i1] != '/' && i1 > 0; i1-- ) {
	}
	char auxChar1 = *(from+i1+1);
	*(from+i1+1) = '\0';
	if ( strlen(from) > 1 ) {
		*(from+i1) = '\0';
	}
	ENTRY aux1;
	setEntry(from, &aux1,0);
	int inodeNumber1 = aux1.inodeNumber;
	*(from+i1) = '/';
	*(from+i1+1) = auxChar1;

	int fd = popen(inodeNumber1);
	if ( fd < 0 ) {
		return -1;
	}
	if ( openFiles[fd].inode->attributes.size < 4*sizeof(ENTRY) ) {
		close(fd);
		return -1;
	}

	lseek(fd, openFiles[fd].inode->attributes.size - 2 * sizeof(ENTRY));
	ENTRY entry;
	read(fd, &entry, sizeof(ENTRY));
	if ( openFiles[fd].inode->attributes.size == 4*sizeof(ENTRY) ) {
		strcpy(entry.name, "");
	}
	char name[30];
	getNameFromPath(from, name);
	lseek(fd, 0);
	ENTRY aux_entry;
	int i;
	
	for ( i = 0 ; read(fd, &aux_entry, sizeof(ENTRY)) == sizeof(ENTRY) ; i++ ) {
		INODE * auxInode = getInode(aux_entry.inodeNumber);
		if ( auxInode->attributes.alive ) {
			if ( !strcmp(aux_entry.name, name) ) {
				ENTRY lastEntry = {"", -1};
				writeNS(fd, i*sizeof(ENTRY), &entry, sizeof(ENTRY));
				writeNS(fd, openFiles[fd].inode->attributes.size - 2 * sizeof(ENTRY), &lastEntry, sizeof(ENTRY));
				openFiles[fd].inode->attributes.size -= sizeof(ENTRY);
				close(fd);
				break;
			}
		}
		free(auxInode);
	}
	getNameFromPath(to, name);
	remove(to);
	int len = strlen(to);
	for ( i = len ; to[i] != '/' && i > 0; i-- ) {
	}
	char auxChar = *(to+i+1);
	*(to+i+1) = '\0';
	if ( strlen(to) > 1 ) {
		*(to+i) = '\0';
	}
	ENTRY newEntry;
	newEntry.inodeNumber = inodeNumber;
	strcpy(newEntry.name, name);

	setEntry(to, &aux,0);
	addToDir(aux, newEntry);
	INODE * inodeFile = getInode(inodeNumber);
	inodeFile->attributes.parentDir = aux.inodeNumber;
	strcpy(inodeFile->attributes.name, name);
	setInode(inodeFile, inodeNumber);
	*(to+i) = '/';
	*(to+i+1) = auxChar;
	free(inodeFile);
	return 0;
}

int
link(char * src, char * dest){
	ENTRY srcEntry;
	setEntry(src, &srcEntry,0);
	if(srcEntry.name[0] == 0){
		printf("No existe el archivo o directorio\n");
		return -1;
	}
	ENTRY destEntry;
	destEntry.inodeNumber = srcEntry.inodeNumber;
	char buff[30];
	getNameFromPath( dest, buff);
//printf("jorge baila conga %s",buff);
	strcpy(destEntry.name, buff);

	int len = strlen(dest), i;
	for ( i = len ; dest[i] != '/' && i > 0; i-- ) {
	}
	char auxChar = *(dest+i+1);
	*(dest+i+1) = '\0';
	if ( strlen(dest) > 1 ) {
		*(dest+i) = '\0';
	}
	setEntry(dest, &srcEntry,0);
	addToDir( srcEntry, destEntry);
	*(dest+i) = '/';
	*(dest+i+1) = auxChar;
	return 0; 
}


/* Retorna el numero de inodo de la version pedidia.
   si no existe retorna -1 */
int
getVersion( int fd, int version){
	if ( fd < 0 || fd > 50 ) {
		return -1;
	}
	if ( openFiles[fd].pid != currentProc->pid ) {
		return -1;
	}
	int versions[200];
	int first = 1, i;
	INODE * inode = openFiles[fd].inode;
	versions[0] = openFiles[fd].inodeNumber;
	i=1;
	while( inode->attributes.prev_version != -1 && i < 200){
		versions[i] = inode->attributes.prev_version;
		if(!first) {
	        	free(inode);
		} else {
			first = 0;
	        }
		inode = getInode(versions[i]);
		i++;
	}
	if( !first ){
		free(inode);
	}   
	if( version > i ){
		printf("No existe dicha version.");
		return -1;
	}   
	return versions[i-version-1];
}

int
history(char * path){
    ENTRY entry;
    setEntry(path, &entry,0);
    if( entry.name[0] == 0 ){
        printf("No existe el archivo o directorio.\n");
        return -1;
    }
    int versions[200];
    int i;
    INODE * inode = getInode(entry.inodeNumber);
    versions[0] = entry.inodeNumber;
    i=1;
    while( inode->attributes.prev_version != -1 && i < 200){
        versions[i] = inode->attributes.prev_version;
        free(inode);
        inode = getInode(versions[i]);
        i++;
    }
    free(inode);
    int j;
    printf("History de %s\n", entry.name);
    for( j = 0; j < i; j++){
	 INODE * inodeAux = getInode( versions[i-j-1]);
	 printf("--------------------------------------------------------------------------------\n");
         printf("- revision nro %d | ", j);
	 printf("name : %s | size : %d | ", inodeAux->attributes.name, inodeAux->attributes.size);
	 printf("modificated : %d:%d:%d %d-%d-%d -\n", inodeAux->attributes.creation_time.hours, inodeAux->attributes.creation_time.minutes, inodeAux->attributes.creation_time.seconds, inodeAux->attributes.creation_time.day, inodeAux->attributes.creation_time.month, inodeAux->attributes.creation_time.year);
	 printf("--------------------------------------------------------------------------------\n");
	 free(inodeAux);
    }
    return 0;
}


int
revert(char * path, int version){
    ENTRY entry;
    if( path == 0 ){
	return -1;
    }
    if( !strcmp(path,"/") ){
	  entry.inodeNumber = 0;
	  strcpy(entry.name,"/");
    }else{
	  if( path[0] != '/' && path[0] != '.'){
	      int xx;
	      for ( xx = strlen(path) ; xx >= 0 ; xx-- ) {
		      path[xx+2] = path[xx];
	      }
	      path[0] = '.';
	      path[1] = '/';
	  }
//printf("entro con -%s-",path);
	  char nameOfFile[100];
//printf("le mande el path de (%s)",path);
	  getNameFromPath(path,nameOfFile);
//printf("nombre del file a revertar (%s)",nameOfFile);
	  int i1;
	  int len1 = strlen(path);
	  for ( i1 = len1 ; path[i1] != '/' && i1 > 0; i1-- ) {
	  }
	  char auxChar1 = *(path+i1+1);
	  *(path+i1+1) = '\0';
	  if ( strlen(path) > 1 ) {
	      *(path+i1) = '\0';
	  }
	  ENTRY cont1;
//printf("llega el path %s-",path);
	  setEntry( path, &cont1,0);
//printf("sale el nombre -%s-",cont1.name);
	  if( cont1.name[0]==0 ){
		printf("El archivo o directorio no existe1.\n");
		return -1;
	   }
	  int j1;
	  int fdcont1 = popen(cont1.inodeNumber);
	  ENTRY aux1;
	  int cantFiles = 0;
	  for( j1=0; read(fdcont1, &aux1, sizeof(ENTRY))==sizeof(ENTRY); j1++){
	      if( !strcmp(aux1.name, nameOfFile) ){
		  cantFiles ++;
		  memcpy((char *)&entry,(char *)&aux1, sizeof(ENTRY));
		  if(cantFiles > 1){
			break;   
		  }
	      }
	  }
	  *(path+i1) = '/';
	  *(path+i1+1) = auxChar1;
	  close(fdcont1);
	  if(cantFiles == 0){
	      printf("El archivo o directorio no existe2.\n");
	      return -1;	     
	  }
	  if(cantFiles != 1){
	      printf("En este directorio hay mas de un archivo o dir con este nombre, por favor realice un forcerm de este archivo para poder hacer esta accion.\n");
	      return -1;	
	  }
    }
    
    if( isDir(entry.inodeNumber) ){
	  ENTRY e;
	  setEntry(path, &e,0);
	  int fd = popen(e.inodeNumber);
	  int j;
	  ENTRY aux;
	  char buff[200];
	  for( j=0; read(fd, &aux, sizeof(ENTRY))==sizeof(ENTRY); j++){
	      if( !strcmp(aux.name, "") || !strcmp(aux.name, ".") || !strcmp(aux.name, "..")){
		  break;
	      }
	      INODE * inode = getInode(aux.inodeNumber);
	      if( inode->attributes.prev_version != -1 ){
		    strcpy(buff, path);
		    strcat(buff, "/");
		    strcat(buff, aux.name);
		    revert(buff, inode->attributes.prev_version);
	      }
	      free(inode);
	  }
//printf("salio pro aca");
	  return 0;
    }
    
    //hasta aca me tieen q qedar el path con el nombre del file, version con la version y entry con el entry del arhcivo a revertar
    int fd = popen(entry.inodeNumber);
    int inodeNumber = getVersion( fd, version);
    INODE * inodeToVers = getInode(inodeNumber);
    int newInodeNumber = inodeNumber;
    // copia el inodo y lo carga en el disco, se le pasa el inodeNumber en el aprametro(el del current) y se deuvelve alli el numb del nuevo
    INODE * newInode = cloneInode(inodeToVers, &newInodeNumber);
    newInode->attributes.prev_version = openFiles[fd].inodeNumber;
    close(fd);
   
    int i;
    int len = strlen(path);
    for ( i = len ; path[i] != '/' && i > 0; i-- ) {
    }
    char auxChar = *(path+i+1);
    *(path+i+1) = '\0';
    if ( strlen(path) > 1 ) {
        *(path+i) = '\0';
    }
    ENTRY cont;
    setEntry( path, &cont,0);
    if( cont.name[0]==0 ){
        printf("ocurrio un error");
        return -1;
    }
    int j;
    int fdcont = popen(cont.inodeNumber);
    ENTRY aux;
    for( j=0; read(fdcont, &aux, sizeof(ENTRY))==sizeof(ENTRY); j++){
        if( !strcmp(aux.name,entry.name) ){
            aux.inodeNumber = newInodeNumber;
	    strcpy( aux.name, entry.name);
	    aux.link = entry.link;
            write( fdcont, j*sizeof(ENTRY), &aux, sizeof(ENTRY));
            break;
	}
    }
    *(path+i) = '/';
    *(path+i+1) = auxChar;
   
    setInode(newInode, newInodeNumber);
   
    return 0;
}


/*setea used en 0 y el mapa de bits de los blokes usados en 0 en el disco*/
int
forceRemoveInode(int inodeNumber){
	if( inodeNumber < 0 || inodeNumber == 0 ){
		return -1;
	}
	/* Borrar los discoc usados del mapa de bits seteandolos en 0 */
	int i = 0;
	INODE * inode = getInode( inodeNumber);
	while( i<BLOCKS_PER_FILE && inode->blocks[i] != -1 ){
		setFreeBlock( inode->blocks[i] );
		i++;
	}
	inode->attributes.free = 1;
	freeInodes[inodeNumber] = 1;
	setInode( inode, inodeNumber);
	
	return 0;
}

/* Me retorna los numeros de inodo del inodo actual y de las versiones anteriores */
void
toRemove(int inodeNumber, int * inodesToRemove, int * size){
	*size = 0;
	inodesToRemove[0] = inodeNumber;
	if( inodeNumber == -1 ){
		return;
	}
	*size = 1;
	int i = 1;
	do{
		 INODE * inode = getInode( inodeNumber);
		 inodeNumber = inode->attributes.prev_version;
		 if(inode->attributes.prev_version != -1){
			inodesToRemove[i] = inode->attributes.prev_version;
			i++;
			(*size) ++;
		 }
		 free(inode);
	}while( inodeNumber != -1 );
	
	return;
}

int
forceRemove( char * path){
	ENTRY entry;
	setEntry(path, &entry,0);
	if( entry.name[0] == 0 ){
		printf("El archivo o directorio no existe vivo.");
		return -1;
	}
	if( !strcmp( entry.name, "/") ){
		printf("No puedes eliminar el directorio root.");
		return -1;
	}
	if( !strcmp( entry.name, "..") || !strcmp( entry.name, ".") ){
		printf("No puedes eliminar este directorio desde esta ubicacion.");
		return -1;
	}
// 	printf("LINK = %d", entry.link);
	if( entry.link != 1 ){
		int sizeToRemove, x, inodesToRemove[100];
		toRemove(entry.inodeNumber, inodesToRemove, &sizeToRemove );
		for(x=0; x < sizeToRemove; x++){
			forceRemoveInode(inodesToRemove[x]);
		}
	}
	
	/* Abrir el padre y removerlo del file */
	int i1;
	int len1 = strlen(path);
	for ( i1 = len1 ; path[i1] != '/' && i1 > 0; i1-- ) {
	}
	char auxChar1= *(path+i1+1);
	*(path+i1+1) = '\0';
	if ( strlen(path) > 1 ) {
		*(path+i1) = '\0';
	}
	char parentPath[100];
	strcpy( parentPath, path);
	*(path+i1) = '/';
	*(path+i1+1) = auxChar1;
	
	ENTRY entryParent;
	setEntry(parentPath, &entryParent,0);
	if( entryParent.name[0] == 0 ){
		printf("Ocurrio un problema inesperado 1.\n");
		return -1;
	}
	int fd = popen(entryParent.inodeNumber);
	ENTRY aux;
	int j;
	if ( fd < 0 ) {
		printf("Ocurrio un problema inesperado 2.\n");
		return -1;
	}
	if ( openFiles[fd].inode->attributes.size < 4*sizeof(ENTRY) ) {
		close(fd);
		return -1;
	}

	lseek(fd, openFiles[fd].inode->attributes.size - 2 * sizeof(ENTRY));
	ENTRY entryLast;
	read(fd, &entryLast, sizeof(ENTRY));
	if ( openFiles[fd].inode->attributes.size == 4*sizeof(ENTRY) ) {
		strcpy(entryLast.name, "");
	}
	lseek(fd, 0);
	for( j=0; read( fd, &aux, sizeof(ENTRY)) == sizeof(ENTRY); j++){
		INODE * inodeToRm = getInode( aux.inodeNumber);
	  	if( inodeToRm->attributes.alive ){
			if( !strcmp(aux.name,entry.name) ){
				free(inodeToRm);
				break;
			}
		}
		free(inodeToRm);
	}
	/*Me qedo el j con la posicion donde esta el entry*/
	ENTRY entryEmpty = { "", -1};
	//printf("entrylast %d %s\n", entryLast);
	writeNS(fd, j*sizeof(ENTRY), &entryLast, sizeof(ENTRY));
	writeNS(fd, openFiles[fd].inode->attributes.size - 2 * sizeof(ENTRY), &entryEmpty, sizeof(ENTRY));
	openFiles[fd].inode->attributes.size -= sizeof(ENTRY);
	close(fd);
	
	return 0;
	
}

int
restore(char * path) {
	ENTRY aux;
	setEntry(path, &aux,1);
	if( aux.name[0] == 0 ){
		printf("No existe archivo\n");
		return -1;
	}
	char name[30];
	getNameFromPath(path, name);
	int i, fd, inodeToRestore = -1, exists = 0;
	ENTRY aux_entry;
	
	int i1;
	int len1 = strlen(path);
	for ( i1 = len1 ; path[i1] != '/' && i1 > 0; i1-- ) {
	}
	char auxChar1= *(path+i1+1);
	*(path+i1+1) = '\0';
	if ( strlen(path) > 1 ) {
	  *(path+i1) = '\0';
	}
	char parentPath[100];
	strcpy( parentPath, path);
	*(path+i1) = '/';
	*(path+i1+1) = auxChar1;
	
	ENTRY entryParent;
	setEntry(parentPath, &entryParent,0);
	fd = popen(entryParent.inodeNumber);
	for ( i = 0 ; read(fd, &aux_entry, sizeof(ENTRY)) == sizeof(ENTRY) ; i++ ) {
		if ( !strcmp(aux_entry.name, "") ) {
			close(fd);
			if ( inodeToRestore != -1 ) {
				INODE * auxInode = getInode(inodeToRestore);
				auxInode->attributes.alive = 1;
				setInode(auxInode, inodeToRestore);
				free(auxInode);
			}
			return 0;
		}
		if ( aux_entry.name[0] != '.' ) {
			if ( !strcmp(aux_entry.name, name) ) {
				if ( exists ) {
				  	close(fd);
					printf("Ya existe un archivo vivo con ese nombre, eliminelo antes.\n");
					return -1;
				}
				inodeToRestore = aux_entry.inodeNumber;
				exists++;
			}
		}
	}
// 	printf("SALI\n");
	close(fd);	

	return -1;	
}
