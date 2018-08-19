// vim:foldmethod=marker:foldlevel=0

#ifndef DOS_H
#define DOS_H

/* BIOS calls */
#define EXPTBL #0xFCC1

/* DOS calls */
#define CONIN   #0x01
#define CONOUT  #0x02
#define CURDRV  #0x19
#define FFIRST  #0x40
#define FNEXT   #0x41
#define OPEN    #0x43
#define CREATE  #0x44
#define CLOSE   #0x45
#define READ    #0x48
#define WRITE   #0x49
#define IOCTL   #0x4B
#define DELETE  #0x4D
#define PARSE   #0x5B
#define TERM    #0x62
#define EXPLAIN #0x66
#define GENV    #0x6B
#define DOSVER  #0x6F

#define DOSCALL  call 5
#define BIOSCALL ld iy,(EXPTBL-1)\
call CALSLT

char get_screen_size(void);
int getchar(void);
int putchar(int c);
char get_current_drive(void);
int open(char *fn, char mode);
int create(char *fn, char mode, char attributes);
int close(int fp);
int read(char* buf, unsigned int size, char fp);
unsigned int write(char* buf, unsigned int size, int fp);
int parse_pathname(char volume_name_flag, char* s);
void exit(int code);
char dosver(void);
void explain(char* buffer, char error_code);
char get_env(char* name, char* buffer, char buffer_size);
char delete_file(char *file);

#endif
