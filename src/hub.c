// vim:foldmethod=marker:foldlevel=0
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MSXHUB_VERSION "0.0.1"

/* BIOS calls */
#define CHGET  #0x009F
#define CHPUT  #0x00A2
#define EXPTBL #0xFCC1

/* DOS calls */
#define CONIN   #0x01
#define CONOUT  #0x02
#define TERM    #0x62
#define DOSVER  #0x6F

#define DOSCALL  call 5
#define BIOSCALL ld iy,(EXPTBL-1)\


/*** functions {{{ ***/

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
  // Check MSX-DOS version >= 2
  if(dosver() < 2) {
    die("This program requires MSX-DOS 2 to run.");
  }
}
/*** helper functions }}} ***/

/*** commands {{{ ***/

void install(char const *package) {
  init();
  printf("TODO: install %s\r\n", package);
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
  } else if (strcicmp(commands[0], "help") == 0) {
    help(commands[1]);
  } else {
    help("");
  }

  return 0;
}

/*** main }}} ***/
