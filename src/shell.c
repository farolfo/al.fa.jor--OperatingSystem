#include "../include/circularBuffer.h"
#include "../include/kernel.h"
#include "../include/libstd.h"
#include "../include/kasm.h"
#include "../include/speaker.h"
#include "../include/kc.h"
#include "../include/string.h"
#include "../include/colormgmt.h"
#include "../include/mempages.h"
#include "../include/sched.h"
#include "../include/defs.h"
#include "../include/filesystem.h"

extern circularBuffer generalBuffer;
extern circularBuffer ioBuffer;
extern int language;
extern PROCESS * currentProc;

int
print2(int argc, char** argv){
  int i;
  for ( i = 0 ; i < 100 ; i++) {
    printf("2\n");
    waiti(3);
  }
  return 0;
}

int
useFile(int argc, char ** argv){
  int fd = open( argv[0]);
  if( fd == -1){
    printf("No existe el archivo.\n");
    return -1;
  }
  char data[4];
  if( 3 == read( fd, &data, sizeof(char)*3) ){
    data[3] = 0;
    if( !strcmp(data,"uno") ){
      printf("Comando a ejecutar con el 1.\n");
    }else if( !strcmp(data, "dos")  ){
      printf("Comando a ejecutar con el 2.\n");
    }else{
      printf("Error en la lectura, el archivo debe contener 'uno' o 'dos'\n");
    }
    close(fd);
    return -1;
  }
  printf("Error en la lectura, el archivo debe contener 'uno' o 'dos'\n");
  
  close(fd);
  
  return 0;
}

int
print1(int argc, char** argv){
  exec("print2", print2, 0, (char **)NULL, 4, 1, 1, 1);
  int i;
  for ( i = 0 ; i < 100 ; i++ ){
	printf("1\n");
	waiti(3);
  }
  return 0;
}

int
testreadPage(int argc, char ** argv){
  readAt(argv[0], stratoi(argv[1]));
  free(argv[0]);
  free(argv[1]);
  free(argv);
  return 0;
}

int
testls(int argc, char ** argv){
	ls(argv[0]);	
	
	free(argv[0]);
	free(argv);
	return 0;
}

int
testrestore(int argc, char ** argv){
  restore(argv[0]);
  
  free(argv[0]);
  free(argv);
  return 0;
}

int
testcat(int argc, char ** argv) {
	//printf("LANZO CAT CON %s, %s\n", argv[0], argv[1]);
	if( ! strcmp( argv[1], "") ){
		printFile(argv[0]);
	}else{
		cat(argv[0],argv[1]);	
	}
	
	free(argv[0]);
	free(argv[1]);
	free(argv);
	
	return 0;
}

int
testrevert(int argc, char ** argv) {
    revert(argv[0], stratoi(argv[1]));
    free(argv[0]);
    free(argv[1]);
    free(argv);
    return 0;
}

int
testhistory(int argc, char ** argv){
    history(argv[0]);
    free(argv[0]);
    free(argv);
    return 0;
}

int 
testln(int argc, char ** argv) {
	link(argv[0], argv[1]);
	free(argv[0]);
	free(argv[1]);
	free(argv);
	return 0;
}

int 
testforcerm(int argc, char ** argv) {
	forceRemove(argv[0]);
	free(argv[0]);
	free(argv);
	return 0;
}

int
testcp(int argc, char ** argv) {
	cp(argv[0],argv[1]);	
	free(argv[0]);
	free(argv[1]);
	free(argv);
	
	return 0;
}

int 
testmv(int argc, char ** argv) {
	mv(argv[0], argv[1]);
	free(argv[0]);
	free(argv[1]);
	free(argv);
	return 0;
}

int 
testrm(int argc, char ** argv) {
	remove(argv[0]);
	free(argv[0]);
	free(argv);
	return 0;
}


int 
testcd(int argc, char ** argv) {
	//printf("CD\n");
	cd(argv[0]);
	free(argv[0]);
	free(argv);
	return 0;
}

int 
testmkdir(int argc, char ** argv) {
	mkdir(argv[0]);
	free(argv[0]);
	free(argv);
	return 0;
}


int
testcwd(int argc, char ** argv){
	printf("%s\n",(currentProc->tty->cwd).name);
	//printf("PCWD :: Name > %s\n",(currentProc->tty->pcwd).name, (currentProc->tty->pcwd).inodeNumber);
	return 0;
}

void printInode(int);
void printDir(int);
int open(char *);

int
testfs(int a, char ** v) {
	int fd = open("/chau");
	int i;
	for (i =0;i<400;i++) {
		write(fd,F_END,"Mister M va a ir  aver Stravaganza, esta muuuuy emocionado.\nAlguien, en cambio, dice que no va a ir a ver a viejas locas, porque se 'suspendio' el concierto. En realidad, va a ir a su nueva banda preferida, Smile en su debut en River. Cristina, tras haber enganiado a su esposa, la de dodgeball. Hoy paso algo muy bueno: me baje del subte y habia gente en la playa en frente del obelisco regalando halls. Despues fui a comer pizza con un boludo que me encontre por ahi. Cuando estaba terminando un indigente empezo a toser a mi lado y la seniora de enfrente y yo nos miramos y nos entendimos. Despues, fui con el mismo boludo a buscar halls. Como el tiene problemas y le gusta el rosita, le pidio a la promotora un halls rozita (notar la z, decirla zezeando).",760);
		printf("%d", i);
		waiti(1);
	}
	return 1;
	
}


int
testpres(int a, char ** v) {
	int sum = 0;
	while( 1){
		printf("%d\n", sum);
		malloc(700);
		sum += 700;
	}
	return 0;
}


int
kill(int argc, char ** argv){
  if(argc != 1 || argv==NULL || argv[0]==NULL){
    printf("ERROR EN EL KILL\n");
    return -1;
  }
  //printAllPids();
  removeProcessByPid( stratoi( argv[0] ) );
  //printAllPids();
  return 0;
}

int
printhola(int a, char** b){
  printf("Hola\n");
  return 0;
}

int
echo(int num, char ** args){
	printf("%s\n",args[0]);
	return 0;
}

void interpretCommand(const char * str) {
	char command[BUFFER_SIZE], arguments[BUFFER_SIZE];
	int inBack = 0;
  	int invalid = 0;
	int isFront = 1;
	
	if( !strcmp(str,"") ){
		return;
	}
  	strspcat(str, command, arguments);
  	if( endsWith(command,'&') ){
		inBack = 1;
		isFront = 0;
		command[(strlen(command)-1)] = '\0';
	}

	
  	if ( !strcmp(command, "echo") ) {
		char ** args = (char **)malloc( sizeof(char *) );
		char * arg0 = (char *)malloc( strlen(arguments) + 1 );
		strcpy( arg0, arguments);
		args[0] = arg0;
		exec("echo", echo, 1, args, 4, isFront, 1, 1);
		free(args);
		free(arg0);
	} else if ( !strcmp(command, "bgcolor") ) {
      		if ( !backgroundColor(arguments) ) {
			printf("Los unicos colores validos son:\n-Black\n-Green\n-Red\n-Cyan\n-Brown\n-Blue\n-Magenta\n-White\n");
      		}
  	} else if ( !strcmp(command, "fgcolor") ) {
      		if ( !letterColor(arguments) ) {
			printf("Los unicos colores validos son:\n-Black\n-Green\n-Red\n-Cyan\n-Brown\n-Blue\n-Magenta\n-White\n");
      		}
  	} else if ( !strcmp(command, "morse") ) {
      		encode(arguments);
  	} else if ( !strcmp(command, "morsespeed") ) {
      		int num = stratoi(arguments);
      		if ( num == -1 ) {
			printf("La nueva velocidad debe ser un numero\n");
      		}
      		if ( num < 1 || num > 3 ) {
			printf("Las unicas velocidades validas son x1, x2, x3\n");
      		}
      		changeSpeed(num);
  	} else if ( !strcmp(command, "testgetchar") ) {
    		putchar(getchar());
  	}  else if ( !strcmp(command, "testpres") ) {
    		exec("tp", testpres, 0, NULL, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "testscanf") ) {
    		int i;
    		char *s;
    		scanf("%i, %s", &i, s);
    		printf("%i", i);
    		printf(s);
  	} else if ( !strcmp(command, "reserv") ){
    		char progName[200];
		char cantMemstr[200];
    		strspcat(arguments, progName, cantMemstr);
		int cantMem = stratoi(cantMemstr);
		if(cantMem <= 0)
	  		printf("Los argumentos deben ser : reserv NAME NUMERO_POSTIVO\n");
		else{
	  		;//malloc_user(progName,cantMem);
		}
  	} else if ( !strcmp(command, "reservzero") ){
        	char progName[200];
        	char cantMemstr[200];
        	strspcat(arguments, progName, cantMemstr);
     		int cantMem = stratoi(cantMemstr);
     		if(cantMem <= 0)
       			printf("Los argumentos deben ser : reservzero NAME NUMERO_POSTIVO\n");
     		else{
       			;//calloc_user(progName,cantMem);
     		}
  	} else if ( !strcmp(command, "free") ){
        	if(strcmp(arguments,""))
			;//free_user(arguments);
		else
	  		printf("Los argumentos deben ser : free NAME\n");
  	} else if ( !strcmp(command, "listmem") && !strcmp(arguments, "") ){
		;//listMemory();
  	} else if ( !strcmp(command, "changelanguage") ) {
      		if ( language == ES_AR ) {
			char * en = "EN_US";
			changeLanguage(EN_US);
			int_80(PRINTSOMEWHERE, 80*2 + 75*2, en, 0); 
      		} else {
			char * es = "ES_AR";
			changeLanguage(ES_AR);
			int_80(PRINTSOMEWHERE, 80*2 + 75*2, es, 0); 
      		}
  	} else if ( !strcmp(command, "visualmorse") ) {
    		char morsemessage[10];
    		int w = 0;
    		while ( arguments[w] != '\0' ) {
      			getMorseCode(arguments[w], morsemessage);
      			printf(morsemessage);
      			w++;
    		}
    		encode(arguments);
    		printf("\n");
  	} else if ( !strcmp(command, "printhola") ) {
		exec("printhola", printhola, 0, (char **)NULL, 0, isFront, 1, 1);
  	} else if ( !strcmp(command, "print1") ) {
		exec("print1", print1, 0, (char **)NULL, 0, isFront, 1, 1);
  	} else if ( !strcmp(command, "print2")) {
		exec("print2", print2, 0, (char **)NULL, 4, isFront, 1, 1);
  	}  else if ( !strcmp(command, "kill")) {
		char pid[200];
		char aux[200];
			strspcat(arguments, pid, aux);
		if( *pid != '\0' && *aux == NULL && stratoi(pid) >= 0 ){
			char ** args = (char **)malloc( sizeof(char *) );
			char * arg0 = (char *)malloc( strlen(pid)*sizeof(char) );
			strcpy( arg0, pid);
			args[0] = arg0;
			//printAllPids();
			//while(1);
			exec("kill", kill, 1, args, 1, isFront, 1, 1);
			free(args);
			free(arg0);
			//printAllPids();
			//while(1);
		}else{
			printf("Los argumentos deben ser : kill PID\n");
			invalid = 1;
		}
  	} else if ( !strcmp(command, "help") ) {
    		printf("Los comandos validos son:\n");
    		printf("-echo STRING\n-bgcolor COLOR\n-fgcolor COLOR\n-reserv STRING NUM\n-reservzero STRING NUM\n-free STRING\n-listmem\n-morse STRING\n-morsespeed STRING\n-visualmorse STRING\n-testscanf\n-testgetchar\n-help\n-changelanguage\n");
  	} else if ( !strcmp(command, "top") ) {
		exec("top", ktop, 0, (char **)NULL, 4, isFront, 1, 1);
	} else if ( !strcmp(command, "ps") ) {
		exec("ps", printAllPids, 0, (char **)NULL, 4, isFront, 1, 1);
	} else if ( !strcmp(command, "testfs") ) {
	  	exec("fs", testfs , 0, (char **)NULL, 4, isFront, 1, 1);
	} else if ( !strcmp(command, "cd")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) );
		char * arg0 = (char *)kmalloc( strlen(pid)*sizeof(char) );
		strcpy( arg0, pid);
		args[0] = arg0;
		printf("ARG[0] %s\n", args[0]);
		exec("cd", testcd, 1, args, 1, isFront, 1, 1);
  	}  else if ( !strcmp(command, "cwd")) {
		exec("cwd", testcwd, 0, (char **)NULL, 4, isFront, 1, 1);
  	} else if ( !strcmp(command, "cat")) {
		//printf("Empezando cat\n");
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
		strcpy( args[0], pid);
		strcpy( args[1], aux);
		exec("cat", testcat, 2, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "cp")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
		strcpy( args[0], pid);
		strcpy( args[1], aux);
		exec("cp", testcp, 2, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "mv")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
		strcpy( args[0], pid);
		strcpy( args[1], aux);
		exec("mv", testmv, 2, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "rm")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		strcpy( args[0], pid);
		exec("rm", testrm, 1, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "ls")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		strcpy( args[0], pid);
		exec("ls", testls, 1, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "ln")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
		strcpy( args[0], pid);
		strcpy( args[1], aux);
		exec("ln", testln, 2, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "mkdir")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		strcpy( args[0], pid);
		exec("mkdir", testmkdir, 1, args, 1, isFront, 1, 1);
  	} else if ( !strcmp(command, "revert")) {
      		char pid[200];
	        char aux[200];
	        strspcat(arguments, pid, aux);
	        char ** args = (char **)kmalloc( sizeof(char *) * 2 );
	        args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
	        args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
	        strcpy( args[0], pid);
	        strcpy( args[1], aux);
	        exec("revert", testrevert, 2, args, 1, isFront, 1, 1);
      } else if ( !strcmp(command, "history")) {
	        char pid[200];
	        char aux[200];
	        strspcat(arguments, pid, aux);
	        char ** args = (char **)kmalloc( sizeof(char *) * 2 );
	        args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
	        args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
	        strcpy( args[0], pid);
	        strcpy( args[1], aux);
	        exec("history", testhistory, 2, args, 1, isFront, 1, 1);
      }  else if ( !strcmp(command, "forcerm")) {
		char pid[200];
		char aux[200];
		strspcat(arguments, pid, aux);
		char ** args = (char **)kmalloc( sizeof(char *) * 2 );
		args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
		strcpy( args[0], pid);
		exec("forcerm", testforcerm, 1, args, 1, isFront, 1, 1);
      } else if ( !strcmp(command, "restore")) {
	char pid[200];
	char aux[200];
	strspcat(arguments, pid, aux);
	char ** args = (char **)kmalloc( sizeof(char *) * 2 );
	args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
	strcpy( args[0], pid);
	exec("restore", testrestore, 1, args, 1, isFront, 1, 1);
      }   else if ( !strcmp(command, "usefile")) {
	char pid[200];
	char aux[200];
	strspcat(arguments, pid, aux);
	char ** args = (char **)kmalloc( sizeof(char *) * 2 );
	args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
	strcpy( args[0], pid);
	exec("usefile", useFile, 1, args, 1, isFront, 1, 1);
      } else if ( !strcmp(command, "readat")) {
	char pid[200];
	char aux[200];
	strspcat(arguments, pid, aux);
	char ** args = (char **)kmalloc( sizeof(char *) * 2 );
	args[0] = kmalloc((strlen(pid) + 1)*sizeof(char));
	args[1] = kmalloc((strlen(aux)+1)*sizeof(char));
	strcpy( args[0], pid);
	strcpy( args[1], aux);
	exec("readpage", testreadPage, 2, args, 1, isFront, 1, 1);
      } else {
    		if ( strcmp(command, "") || strcmp(arguments, "") ) {
      			printf("comando invalido\n");
      			invalid = 1;
    		}
  	}

  	if(!invalid && !inBack){
		block(BLOCK_WAITCHILD);
		yield();
	}


}

int
shell(int argc, char ** argv){
 int tickpos;
 int character;
 int initialpos;
 //initialize(&generalBuffer);
 char command[BUFFER_SIZE];
  int_80(CURSOR, 0, 0,tickpos/2);
  if(tickpos%(80*2) != 0)  {
    tickpos +=  (( 80*2 ) - tickpos%(80*2));
  }
  printf("Shell:>>");
  int_80(TICKPOS, 0, &tickpos, 0); 
  initialpos = tickpos;
  int_80(CURSOR, 0, 0,tickpos/2);

  while(1){
          character = next(&(currentProc->tty->inputBuffer));   
      switch(character) {
	case '\n':
	 getStringFromBegining(&(currentProc->tty->inputBuffer), command);
	 putchar('\n');
	 interpretCommand(command); 
	 printf("Shell:>>");
	 int_80(TICKPOS, 0, &tickpos, 0); 
	 initialpos = tickpos;
	 int_80(CURSOR, 0, 0,tickpos/2);break;
	case '\b':
	  int_80(BACKSPACE,0,0, initialpos);break;
	  break;
	case '\0':
	  break;
	default:
	  putchar(character);break;
    }
  }
  return 0;
}
