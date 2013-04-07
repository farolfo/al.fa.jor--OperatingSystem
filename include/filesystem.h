/***************************************************
  FS.h
	
****************************************************/

#ifndef _filesystem_
#define _filesystem_

#define BLOCKS_PER_FILE 109

typedef struct TIME {
	unsigned int seconds, minutes, hours, day, month, year;
} TIME;


typedef struct ATTR {
	int size;
	char alive;
	int prev_version;
	TIME creation_time;
	char name[30];
	int parentDir;
	char free;
} ATTR;

typedef int BLOCKS[BLOCKS_PER_FILE];

typedef struct INODE {
	ATTR attributes;
	BLOCKS blocks;
} INODE;

typedef struct ENTRY {
	char name[30];
	int inodeNumber;
	int link;
} ENTRY;

void init_fs();
int mkdir(char*);
void ls(char *);
void cd(char *);
int readPage(int);
int write(int, int, void*, int);
void printFile(char *);
int cat(char *, char *);
int cp(char*, char*);
int mv(char*, char*);
int remove(char *);
int link(char *, char*);
int revert(char *, int);
int history(char *);
int forceRemove(char *);
int restore(char *);
int open(char *);
int close(int);
int read(int, void *, int);
int readAt(char *, int);
int lseek(int,int);
#define F_END -1
#define F_START 0

#endif
