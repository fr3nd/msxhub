// vim:foldmethod=marker:foldlevel=0
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MSXHUB_VERSION "0.0.1"

#define DEBUG 1

/* BIOS calls */
#define EXPTBL #0xFCC1

/* DOS calls */
#define CONIN   #0x01
#define CONOUT  #0x02
#define CURDRV  #0x19
#define OPEN    #0x43
#define CREATE  #0x44
#define CLOSE   #0x45
#define READ    #0x48
#define WRITE   #0x49
#define PARSE   #0x5B
#define TERM    #0x62
#define EXPLAIN #0x66
#define GENV    #0x6B
#define DOSVER  #0x6F
#define _EOF    0xC7

/* DOS errors */
#define NOFIL   0xD7
#define IATTR   0xCF
#define DIRX    0xCC

/* open/create flags */
#define  O_RDWR     0x00
#define  O_RDONLY   0x01
#define  O_WRONLY   0x02
#define  O_INHERIT  0x04

/* file attributes */
#define ATTR_READ_ONLY (1)
#define ATTR_DIRECTORY (1 << 4)
#define ATTR_ARCHIVE   (1 << 5)

#define DOSCALL  call 5
#define BIOSCALL ld iy,(EXPTBL-1)\


/*** global variables {{{ ***/
char configpath[255] = { '\0' };
/*** global variables }}} ***/

/*** DOS functions {{{ ***/

int getchar(void) __naked {
  __asm
    push ix

    ld c,CONIN
    DOSCALL
    ld h, #0x00

    pop ix
    ret
  __endasm;
}

int putchar(int c) __naked {
  c;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld e,(ix)
    ld c,CONOUT
    DOSCALL

    pop ix
    ret
  __endasm;
}

char get_current_drive(void) __naked {
  __asm
    push ix

    ld c, CURDRV
    DOSCALL

    ld h, #0x00
    ld l, a

    pop ix
    ret
  __endasm;
}

int open(char *fn, char mode) __naked {
  fn;
  mode;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld e,0(ix)
    ld d,1(ix)
    ld a,2(ix)
    ld c, OPEN
    DOSCALL

    cp #0
    jr z, open_noerror$
    ld h, #0xff
    ld l, a
    jp open_error$
  open_noerror$:
    ld h, #0x00
    ld l, b
  open_error$:
    pop ix
    ret
  __endasm;
}

int create(char *fn, char mode, char attributes) __naked {
  fn;
  mode;
  attributes;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld e,0(ix)
    ld d,1(ix)
    ld a,2(ix)
    ld b,3(ix)
    ld c, CREATE
    DOSCALL

    cp #0
    jr z, open_noerror$
    ld h, #0xff
    ld l, a
    jp open_error$
  create_noerror$:
    ld h, #0x00
    ld l, b
  create_error$:
    pop ix
    ret
  __endasm;
}

int close(int fp) __naked {
  fp;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld b,(ix)
    ld c, CLOSE
    DOSCALL

    ld h, #0x00
    ld l,a

    pop ix
    ret
  __endasm;
}

int read(char* buf, unsigned int size, char fp) __naked {
  buf;
  size;
  fp;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld e,0(ix)
    ld d,1(ix)
    ld l,2(ix)
    ld h,3(ix)
    ld b,4(ix)
    ld c, READ
    DOSCALL

    cp #0
    jr z, read_noerror$
    ld h, #0xFF
    ld l, #0xFF
  read_noerror$:
    pop ix
    ret
  __endasm;
}

unsigned int write(char* buf, unsigned int size, int fp) __naked {
  buf;
  size;
  fp;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld e,0(ix)
    ld d,1(ix)
    ld l,2(ix)
    ld h,3(ix)
    ld b,4(ix)
    ld c, WRITE
    DOSCALL

    cp #0
    jr z, write_noerror$
    ld h, #0xFF
    ld l, #0xFF
  write_noerror$:
    pop ix
    ret
  __endasm;
}

int parse_pathname(char volume_name_flag, char* s) __naked {
  volume_name_flag;
  s;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld b,0(ix)
    ld e,1(ix)
    ld d,2(ix)

    ld c, PARSE
    DOSCALL

    pop ix
    ret
  __endasm;
}

void exit(int code) __naked {
  code;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld b,(ix)
    ld c, TERM
    DOSCALL

    pop ix
    ret
  __endasm;
}

char dosver(void) __naked {
  __asm
    push ix

    ld c, DOSVER
    DOSCALL

    ld h, #0x00
    ld l, b

    pop ix
    ret
  __endasm;
}

void explain(char* buffer, char error_code) __naked {
  error_code;
  buffer;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld e,0(ix)
    ld d,1(ix)
    ld b,2(ix)

    ld c, EXPLAIN
    DOSCALL

    ld 0(ix),d
    ld 1(ix),e

    pop ix
    ret
  __endasm;
}

char get_env(char* name, char* buffer, char buffer_size) __naked {
  name;
  buffer;
  buffer_size;
  __asm
    push ix
    ld ix,#4
    add ix,sp

    ld l,0(ix)
    ld h,1(ix)
    ld e,2(ix)
    ld d,3(ix)
    ld b,4(ix)

    ld c, GENV
    DOSCALL

    ld 0(ix),l
    ld 1(ix),h
    ld 2(ix),d
    ld 3(ix),e
    ld 4(ix),a

    pop ix
    ret
  __endasm;
}


/*** DOS functions }}} ***/

/*** functions {{{ ***/


void debug(const char *s, ...) {
  if (DEBUG == 1) {
    va_list ap;
    va_start(ap, s);
    printf("*** DEBUG: ");
    vprintf(s, ap);
    printf("\r\n");
  }
}

// Based on https://github.com/svn2github/sdcc/blob/master/sdcc/device/lib/gets.c
char* gets (char *s) {
  char c;
  unsigned int count = 0;

  while (1) {
    c = getchar ();
    switch(c) {
      case '\b': /* backspace */
        if (count > 0) {
          printf("\33K");
          --s;
          --count;
        } else {
          putchar('\34');
        }
        break;

      case '\n':
      case '\r': /* CR or LF */
        putchar ('\r');
        putchar ('\n');
        *s = 0;
        return s;

      default:
        *s++ = c;
        ++count;
        break;
    }
  }
}

void die(const char *s, ...) {
  va_list ap;
  va_start(ap, s);
  printf("*** ");
  vprintf(s, ap);
  printf("\r\n");
  exit(1);
}

int strcicmp(char const *a, char const *b) {
  for (;; a++, b++) {
    int d = tolower(*a) - tolower(*b);
    if (d != 0 || !*a)
      return d;
  }
}

/*** functions }}} ***/

/*** helper functions{{{ ***/
void usage() {
  printf("hub command\r\n");
}

void init(void) {
  int p;

  // Check MSX-DOS version >= 2
  if(dosver() < 2) {
    die("This program requires MSX-DOS 2 to run.");
  }

  // Get the full path of the program and extract only the directory
  get_env("PROGRAM", configpath, sizeof(configpath));
  p = parse_pathname(0, configpath);
  *((char*)p) = '\0';
  strcat(configpath, "CONFIG");
}

void save_config(char* filename, char* value) {
  int fp;
  int n;
  char buffer[255] = { '\0' };

  strcpy(buffer, configpath);
  strcat(buffer, "\\");
  strcat(buffer, filename);
  fp = create(buffer, O_RDWR, 0x00);

  // Error is in the least significative byte of p
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    printf("Error saving configuration %s: 0x%X\r\n", buffer, n);
    explain(buffer, n);
    die("%s", buffer);
  }

  n = write(value, strlen(value), fp);
  buffer[0] = 0x1a;
  write(buffer, 1, fp);
  close(fp);
}

const char* get_config(char* filename) {
  int fp;
  int n;
  char buffer[255] = { '\0' };

  strcpy(buffer, configpath);
  strcat(buffer, "\\");
  strcat(buffer, filename);
  fp = open(buffer, O_RDONLY);

  // Error is in the least significative byte of p
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    printf("Error reading configuration %s: 0x%X\r\n", buffer, n);
    explain(buffer, n);
    die("%s", buffer);
  }

  n = read(buffer, sizeof(buffer), fp);
  close(fp);
  debug("N: %d", n);
  buffer[n] = '\0';
  return buffer;
}

/*** helper functions }}} ***/

/*** commands {{{ ***/

void install(char const *package) {
  printf("TODO: install %s\r\n", package);
}

void configure(void) {
  char buffer[255] = { '\0' };
  char progsdir[255] = { '\0' };
  char c;
  int fp, n;

  printf("Welcome to MSX Hub!\r\n\n");
  printf("This wizard will guide you through the configuration process.\r\n\n");

  printf("* Configuration directory: %s\r\n", configpath);

  // Create config dir if it doesn't exist
  fp = create(configpath, O_RDWR, ATTR_DIRECTORY);
  // Error is in the least significative byte of fp
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    if (n == DIRX) {
      printf("Configuration directory already exists. Continue? (y/N)\r\n");
      c = tolower(getchar());
      printf("\r\n");
      if (c != 'y') {
        die("Aborted");
      }
    } else {
      printf("Error creating configuration directory: 0x%X\r\n", n);
      explain(buffer, n);
      die("%s", buffer);
    }
  }

  // Guess the default install directory
  progsdir[0] = configpath[0];
  progsdir[1] = '\0';
  strcat(progsdir, ":\\PROGRAMS");

  // Configure where to install programs
  printf("Where are programs going to be installed? (Default: %s)\r\n", progsdir);
  gets(buffer);
  if (buffer[0] != '\0') {
    strcpy(progsdir, buffer);
  }
  save_config("PROGSDIR", progsdir);

  save_config("BASEURL", "http://msxhub.com/files/");

  printf("Done! MSX Hub configured properly.\r\n");
}

void help(char const *command) {
  usage();
  printf("TODO: help message\r\n");
}

/*** commands }}} ***/

/*** main {{{ ***/

int main(char **argv, int argc) {

  int i, n;
  char commands[3][20] = {{'\0'}, {'\0'}, {'\0'}};

  init();

  n = 0;
  for (i = 0; i < argc; i++) {
    if (argv[i][0] == '/') {
      switch (argv[i][1]) {
        case 'h':
          help("");
          break;
        default:
          usage();
          die("ERROR: Parameter not recognized");
          break;
      }
    } else {
      strcpy(commands[n], argv[i]);
      n++;
    }
  }

  if (strcicmp(commands[0], "install") == 0) {
    install(commands[1]);
  } else if (strcicmp(commands[0], "configure") == 0) {
    configure();
  } else if (strcicmp(commands[0], "help") == 0) {
    help(commands[1]);
  } else {
    help("");
  }

  //printf("CONFIG: %s\r\n", get_config("PROGSDIR"));

  return 0;
}

/*** main }}} ***/
