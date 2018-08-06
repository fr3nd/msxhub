// vim:foldmethod=marker:foldlevel=0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "asm.h"

#define MSXHUB_VERSION "0.0.1"

// Enable or disable debug messages
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
call CALSLT
#define TCP_WAIT() UnapiCall(code_block, TCPIP_WAIT, &regs, REGS_NONE, REGS_NONE)

#define HTTP_DEFAULT_PORT (80)

/* UNAPI functions */
#define UNAPI_GET_INFO  0
#define TCPIP_GET_CAPAB 1
#define TCPIP_NET_STATE 3
#define TCPIP_DNS_Q     6
#define TCPIP_DNS_S     7
#define TCPIP_TCP_OPEN  13
#define TCPIP_TCP_CLOSE 14
#define TCPIP_TCP_ABORT 15
#define TCPIP_TCP_STATE 16
#define TCPIP_TCP_SEND  17
#define TCPIP_TCP_RCV   18
#define TCPIP_WAIT      29

/* UNAPI error codes */
#define ERR_OK           0
#define ERR_NOT_IMP      1
#define ERR_NO_NETWORK   2
#define ERR_NO_DATA      3
#define ERR_INV_PARAM    4
#define ERR_QUERY_EXISTS 5
#define ERR_INV_IP       6
#define ERR_NO_DNS       7
#define ERR_DNS          8
#define ERR_NO_FREE_CONN 9
#define ERR_CONN_EXISTS  10
#define ERR_NO_CONN      11
#define ERR_CONN_STATE   12
#define ERR_BUFFER       13
#define ERR_LARGE_DGRAM  14
#define ERR_INV_OPER     15

#define ERR_DNS_S              20
#define ERR_DNS_S_UNKNOWN      0
#define ERR_DNS_S_INV_PACKET   1
#define ERR_DNS_S_SERVER_FAIL  2
#define ERR_DNS_S_NO_HOST      3
#define ERR_DNS_S_UNSUPPORTED  4
#define ERR_DNS_S_REFUSED      5
#define ERR_DNS_S_NO_REPLY     16
#define ERR_DNS_S_TIMEOUT      17
#define ERR_DNS_S_CANCELLED    18
#define ERR_DNS_S_NETWORK_LOST 19
#define ERR_DNS_S_BAD_RESPONSE 20
#define ERR_DNS_S_TRUNCATED    21

#define ERR_CUSTOM         100
#define ERR_CUSTOM_TIMEOUT 0

#define TCP_TIMEOUT 10
#define TCP_BUFFER_SIZE (1024)
#define TCPOUT_STEP_SIZE (512)
#define TICKS_TO_WAIT (20*60)
#define SYSTIMER ((uint*)0xFC9E)

typedef char ip_addr[4];

typedef struct {
    ip_addr remote_ip;
    unsigned int remote_port;
    unsigned int local_port;
    int user_timeout;
    char flags;
} unapi_connection_parameters;

/*** global variables {{{ ***/
char configpath[255] = { '\0' };
char progsdir[255] = { '\0' };
char baseurl[255] = { '\0' };
Z80_registers regs;
unapi_code_block *code_block;
unapi_connection_parameters *parameters;
/*** global variables }}} ***/

/*** prototypes {{{ ***/
void die(const char *s, ...);
void debug(const char *s, ...);
/*** prototypes }}} ***/

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

/*** UNAPI functions {{{ ***/

char *unapi_strerror (int errnum) {
  switch(errnum) {
    case ERR_OK:
      return "Operation completed successfully.";
    case ERR_NOT_IMP:
      return "Capability not implemented in this TCP/IP UNAPI implementation.";
    case ERR_NO_NETWORK:
      return "No network connection available.";
    case ERR_NO_DATA:
      return "No incoming data available.";
    case ERR_INV_PARAM:
      return "Invalid input parameter.";
    case ERR_QUERY_EXISTS:
      return "Another query is already in progress.";
    case ERR_INV_IP:
      return "Invalid IP address.";
    case ERR_NO_DNS:
      return "No DNS servers are configured.";
    case ERR_NO_FREE_CONN:
      return "No free connections available.";
    case ERR_CONN_EXISTS:
      return "Connection already exists.";
    case ERR_NO_CONN:
      return "Connection does not exist or connection lost.";
    case ERR_CONN_STATE:
      return "Invalid connection state.";
    case ERR_BUFFER:
      return "Insufficient output buffer space.";
    case ERR_LARGE_DGRAM:
      return "Datagram is too large.";
    case ERR_INV_OPER:
      return "Invalid operation.";
    // >= 20 ERR_DNS DNS server error
    case ERR_DNS_S_UNKNOWN + ERR_DNS_S:
      return "DNS server error: Unknown error.";
    case ERR_DNS_S_INV_PACKET + ERR_DNS_S:
      return "DNS server error: Invalid query packet format.";
    case ERR_DNS_S_SERVER_FAIL + ERR_DNS_S:
      return "DNS server error: DNS server failure.";
    case ERR_DNS_S_NO_HOST + ERR_DNS_S:
      return "DNS server error: The specified host name does not exist.";
    case ERR_DNS_S_UNSUPPORTED + ERR_DNS_S:
      return "DNS server error: The DNS server does not support this kind of query.";
    case ERR_DNS_S_REFUSED + ERR_DNS_S:
      return "DNS server error: Query refused.";
    case ERR_DNS_S_NO_REPLY + ERR_DNS_S:
      return "DNS server error: One of the queried DNS servers did not reply.";
    case ERR_DNS_S_TIMEOUT + ERR_DNS_S:
      return "DNS server error: Total process timeout expired.";
    case ERR_DNS_S_CANCELLED + ERR_DNS_S:
      return "DNS server error: Query cancelled by the user.";
    case ERR_DNS_S_NETWORK_LOST + ERR_DNS_S:
      return "DNS server error: Network connectivity was lost during the process.";
    case ERR_DNS_S_BAD_RESPONSE + ERR_DNS_S:
      return "DNS server error: The obtained reply did not contain REPLY nor AUTHORITATIVE.";
    case ERR_DNS_S_TRUNCATED + ERR_DNS_S:
      return "DNS server error: The obtained reply is truncated.";
    // >= 100 Custom errors
    case ERR_CUSTOM_TIMEOUT + ERR_CUSTOM:
      return "Connection timeout.";
  }
  if (errnum >= ERR_DNS_S) {
    return "DNS server error: Undefined error.";
  } else {
    return "Invalid error.";
  }
}

char getaddrinfo(char *hostname, ip_addr ip) {
  regs.Words.HL = (int)hostname;
  regs.Bytes.B = 0;
  UnapiCall(code_block, TCPIP_DNS_Q, &regs, REGS_MAIN, REGS_MAIN);
  if(regs.Bytes.A != 0) {
    return regs.Bytes.A;
  }

  do {
    // AbortIfEscIsPressed();
    TCP_WAIT();
    regs.Bytes.B = 0;
    UnapiCall(code_block, TCPIP_DNS_S, &regs, REGS_MAIN, REGS_MAIN);
  } while (regs.Bytes.A == 0 && regs.Bytes.B == 1);


  // If there is an error during the query, return 20+ the query error
  // This way it can be easily detected and extracted
  if(regs.Bytes.A != 0) {
    return ERR_DNS_S + regs.Bytes.B;
  }

  debug("resolve: %s %d.%d.%d.%d", hostname, regs.Bytes.L, regs.Bytes.H, regs.Bytes.E, regs.Bytes.D);
  ip[0] = regs.Bytes.L;
  ip[1] = regs.Bytes.H;
  ip[2] = regs.Bytes.E;
  ip[3] = regs.Bytes.D;
  return 0;
}

char tcp_connect(char *conn, char *hostname, unsigned int port) {
  int n;
  int sys_timer_hold;

  debug("Connecting to %s:%d... ", hostname, port);
  getaddrinfo(hostname, parameters->remote_ip);
  parameters->remote_port = port;
  parameters->local_port = 0xFFFF;
  parameters->user_timeout = TCP_TIMEOUT;
  parameters->flags = 0;

  debug("Connection parameters:");
  debug("- remote_ip: %d.%d.%d.%d",
      parameters->remote_ip[0],
      parameters->remote_ip[1],
      parameters->remote_ip[2],
      parameters->remote_ip[3]);
  debug("- remote_port: %d", parameters->remote_port);
  debug("- local_port: 0x%X", parameters->local_port);
  debug("- user_timeout: %d", parameters->user_timeout);
  debug("- flags: 0x%X", parameters->flags);

  regs.Words.HL = (int)parameters;
  debug("HL: %X", regs.Words.HL);

  // TODO FIX! invalid input parameter...
  UnapiCall(code_block, TCPIP_TCP_OPEN, &regs, REGS_MAIN, REGS_MAIN);
  if(regs.Bytes.A == (char)ERR_NO_FREE_CONN) {
    regs.Bytes.B = 0;
    UnapiCall(code_block, TCPIP_TCP_ABORT, &regs, REGS_MAIN, REGS_NONE);
    regs.Words.HL = (int)parameters;
    UnapiCall(code_block, TCPIP_TCP_OPEN, &regs, REGS_MAIN, REGS_MAIN);
  }

  if(regs.Bytes.A != (char)ERR_OK) {
    return regs.Bytes.A;
  }
  *conn = regs.Bytes.B;

  n = 0;
  do {
    // AbortIfEscIsPressed();
    sys_timer_hold = *SYSTIMER;
    TCP_WAIT();
    while(*SYSTIMER == sys_timer_hold);
    n++;
    if(n >= TICKS_TO_WAIT) {
      return ERR_CUSTOM + ERR_CUSTOM_TIMEOUT;
    }
    regs.Bytes.B = *conn;
    regs.Words.HL = 0;
    UnapiCall(code_block, TCPIP_TCP_STATE, &regs, REGS_MAIN, REGS_MAIN);
  } while((regs.Bytes.A) == 0 && (regs.Bytes.B != 4));

  if(regs.Bytes.A != 0) {
    debug("Error connecting: %i", regs.Bytes.A);
    return regs.Bytes.A;
  }

  debug("Connection established");
  return ERR_OK;

}

char tcp_send(char *conn, char *data) {
  int data_size;

  data_size = strlen(data);

  do {
    do {
      regs.Bytes.B = *conn;
      regs.Words.DE = (int)data;
      regs.Words.HL = data_size > TCPOUT_STEP_SIZE ? TCPOUT_STEP_SIZE : data_size;
      regs.Bytes.C = 1;
      UnapiCall(code_block, TCPIP_TCP_SEND, &regs, REGS_MAIN, REGS_AF);
      if(regs.Bytes.A == ERR_BUFFER) {
        TCP_WAIT();
        regs.Bytes.A = ERR_BUFFER;
      }
    } while(regs.Bytes.A == ERR_BUFFER);
    debug("tcp_send: sent %i bytes\r\n", regs.Words.HL);
    data_size -= TCPOUT_STEP_SIZE;
    data += regs.Words.HL;   //Unmodified since REGS_AF was used for output
  } while(data_size > 0 && regs.Bytes.A == 0);

  return regs.Bytes.A;

}

char tcp_close(char *conn) {
  if(conn != 0) {
    regs.Bytes.B = *conn;
    UnapiCall(code_block, TCPIP_TCP_CLOSE, &regs, REGS_MAIN, REGS_NONE);
    conn = 0;
  }
  return ERR_OK;
}


void init_unapi(void) {
  ip_addr ip;
  char err_code;
  char conn = 0;

  // FIXME Not sure why it doesn't work with malloc...
  //code_block = malloc(sizeof(unapi_code_block));
  //parameters = malloc(sizeof(unapi_connection_parameters));
  code_block = (unapi_code_block*)0x8300;
  parameters = (unapi_connection_parameters*)0x8400;

  err_code = UnapiGetCount("TCP/IP");
  if(err_code == 0) {
    die("An UNAPI compatible network card is required to run this program.");
  }
  UnapiBuildCodeBlock(NULL, 1, code_block);

  regs.Bytes.B = 1;
  UnapiCall(code_block, TCPIP_GET_CAPAB, &regs, REGS_MAIN, REGS_MAIN);
  if((regs.Bytes.L & (1 << 3)) == 0) {
    die("This TCP/IP implementation does not support active TCP connections.");
  }

  regs.Bytes.B = 0;
  UnapiCall(code_block, TCPIP_TCP_ABORT, &regs, REGS_MAIN, REGS_MAIN);
  debug("TCP/IP UNAPI initialized OK");
  err_code = tcp_connect(&conn, "192.168.1.110", 8001);
  if (err_code != 0) {
    die(unapi_strerror(err_code));
  };
  err_code = tcp_send(&conn, "GET /TEST\r\n");
  if (err_code != 0) {
    die(unapi_strerror(err_code));
  };
}


/*** UNAPI functions }}} ***/

/*** functions {{{ ***/

void debug(const char *s, ...) {
  #if DEBUG == 1
  va_list ap;
  va_start(ap, s);
  printf("*** DEBUG: ");
  vprintf(s, ap);
  printf("\r\n");
  va_end(ap);
  #endif
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
  debug("Configpath: %s", configpath);
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
  debug("Save config: %s = %s", filename, value);
  close(fp);
}

const char* get_config(char* filename) {
  int fp;
  int n;
  char buffer[255] = { '\0' };

  // TODO ENV variable should have precedence over file

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
  buffer[n] = '\0';
  debug("Read config: %s = %s", filename, buffer);
  return buffer;
}

void read_config(void) {
  strcpy(progsdir, get_config("progsdir"));
  strcpy(baseurl, get_config("BASEURL"));
}

/*** helper functions }}} ***/

/*** commands {{{ ***/

void install(char const *package) {
  read_config();
  init_unapi();
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
