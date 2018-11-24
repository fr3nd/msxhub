// vim:foldmethod=marker:foldlevel=0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "globals.h"
#include "asm.h"
#include "dos.h"
#include "morestr.h"
#include "base64.h"

/*** prototypes {{{ ***/
void die(const char *s, ...);
void progress_bar(unsigned long current, unsigned long total, char size, char *unit);
void debug(const char *s, ...);
void debug_nocrlf(const char *s, ...);
void trim(char * s);
void tolower_str(char *str);
void toupper_str(char *str);
unsigned long hexstr2ul(char *str);
void abort_if_esc_is_pressed(void);
void parse_url(char *url, url *parsed_url);
/*** prototypes }}} ***/

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

void run_or_die(int err_code) {
  if (err_code != 0) {
    die(unapi_strerror(err_code));
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
    abort_if_esc_is_pressed();
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
    abort_if_esc_is_pressed();
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
      abort_if_esc_is_pressed();
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
    debug_nocrlf("> %s", data);
    data_size -= TCPOUT_STEP_SIZE;
    data += regs.Words.HL;   //Unmodified since REGS_AF was used for output
  } while(data_size > 0 && regs.Bytes.A == 0);

  return regs.Bytes.A;

}

void parse_response(char *header) {
  char buffer[MAX_HEADER_SIZE];
  char *token;
  int n;

  strcpy(buffer, header);

  n = 0;
  token = strtok(buffer," ");
  while( token != NULL ) {
    if (n == 1) {
      headers_info.status_code = (unsigned int)atoi(token);
    }
    token = strtok(NULL, " ");
    n++;
  }
}

void parse_header(char *header, char *title, char *value) {
  char buffer[MAX_HEADER_SIZE];
  char *token;
  int n;

  strcpy(buffer, header);

  n = 0;
  token = strtok(buffer,":");
  while( token != NULL ) {
    if (n == 0) {
      trim(token);
      strcpy(title, token);
    } else {
      trim(token);
      strcpy(value, token);
    }
    token = strtok(NULL, "\0");
    n++;
  }
}

char tcp_get(char *conn, data_buffer_t *data_buffer) {
  int n;
  int sys_timer_hold;

  n = 0;

  data_buffer->size = 0;
  data_buffer->current_pos = 0;
  data_buffer->no_more_data = 0;
  data_buffer->data[0] = '\n';

  while(1) {
    abort_if_esc_is_pressed();
    sys_timer_hold = *SYSTIMER;
    TCP_WAIT();
    while(*SYSTIMER == sys_timer_hold);
    n++;
    if(n >= TICKS_TO_WAIT) {
      return ERR_CUSTOM + ERR_CUSTOM_TIMEOUT;
    }
    regs.Bytes.B = *conn;
    regs.Words.DE = (int)data_buffer->data;
    regs.Words.HL = TCP_BUFFER_SIZE;
    UnapiCall(code_block, TCPIP_TCP_RCV, &regs, REGS_MAIN, REGS_MAIN);
    if(regs.Bytes.A != 0) {
      return regs.Bytes.A;
    } else {
      if (regs.UWords.BC != 0) {
        // TODO Implement urgent data
        data_buffer->size = regs.UWords.BC;
        debug("tcp_get: received %i bytes", regs.UWords.BC);
        return 0;
      }
    }
  }
}

char tcp_close(char *conn) {
  if(conn != 0) {
    regs.Bytes.B = *conn;
    UnapiCall(code_block, TCPIP_TCP_CLOSE, &regs, REGS_MAIN, REGS_NONE);
    *conn = 0;
  }
  // TODO Handle errors
  return ERR_OK;
}

void get_unapi_version_string(char *unapiver) {
  char c;
  byte version_main;
  byte version_sec;
  uint name_address;
  char buffer[80];
  int n;

  UnapiCall(code_block, UNAPI_GET_INFO, &regs, REGS_NONE, REGS_MAIN);
  version_main = regs.Bytes.B;
  version_sec = regs.Bytes.C;
  name_address = regs.UWords.HL;

  n = 0;
  do {
    c = UnapiRead(code_block, name_address);
    buffer[n] = c;
    name_address++;
    n++;
  } while (c != '\0');

  sprintf(unapiver, "%s v%i.%i", buffer, version_main, version_sec);
  debug("%s", unapiver);
}

void init_headers_info(void) {
  headers_info.content_length = 0;
  headers_info.status_code = 0;
  headers_info.is_chunked = 0;
  headers_info.is_redirect = 0;
  headers_info.location[0] = '\0';
}

void init_unapi(void) {
  char err_code;

  // FIXME Not sure why it doesn't work with malloc...
  //code_block = malloc(sizeof(unapi_code_block));
  //parameters = malloc(sizeof(unapi_connection_parameters));
  code_block = (unapi_code_block*)0xA200;
  parameters = (unapi_connection_parameters*)0xA300;
  data_buffer = (data_buffer_t*)0xA400;

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
  get_unapi_version_string(unapiver);
}

/*** UNAPI functions }}} ***/

/*** HTTP functions {{{ ***/

char http_send(char *conn, char *hostname, char *username, char *password, unsigned int port, char *method, char *path) {
  char buffer[128];
  char credentials[128];
  char credentialsb64[128];
  int l;

  init_headers_info();

  run_or_die(tcp_connect(conn, hostname, port));

  sprintf(buffer, "%s %s HTTP/1.1\r\n", method, path);
  run_or_die(tcp_send(conn, buffer));

  sprintf(buffer, "Host: %s\r\n", hostname);
  run_or_die(tcp_send(conn, buffer));

  if (username[0] != '\0' && password[0] != '\0') {
    Base64Init(0);
    sprintf(credentials, "%s:%s", username, password);
    l = Base64EncodeChunk(credentials, credentialsb64, strlen(credentials), 1);
    credentialsb64[l] = '\0';
    sprintf(buffer, "Authorization: Basic %s\r\n", credentialsb64);
    run_or_die(tcp_send(conn, buffer));
  }

  sprintf(buffer, "User-Agent: MSXHub/%s (MSX-DOS %i; %s)\r\n", MSXHUB_VERSION, msxdosver, unapiver);
  run_or_die(tcp_send(conn, buffer));

  run_or_die(tcp_send(conn, "Accept: */*\r\n"));
  run_or_die(tcp_send(conn, "Connection: close\r\n"));
  run_or_die(tcp_send(conn, "Accept-Encoding: identity\r\n"));

  run_or_die(tcp_send(conn, "\r\n"));
  return 0;
}

char http_get_headers(char *conn) {
  int m, x;
  char header[MAX_HEADER_SIZE];
  char title[32];
  char value[MAX_HEADER_SIZE];
  char empty_line;

  x = 0;
  do { // Repeat until an empty line is detected (end of headers)
    do { // Repeat until there is no more data
      run_or_die(tcp_get(conn, data_buffer));
      data_buffer->current_pos = 0;
      data_buffer->chunk_size = 0;
      data_buffer->chunked_data_available = 0;
      empty_line = 0;
      do {
        m = 0;
        while (data_buffer->data[data_buffer->current_pos] != '\n') { // while not end of header
          if (data_buffer->data[data_buffer->current_pos] != '\r') { // ignore \r
            header[m] = data_buffer->data[data_buffer->current_pos];
            m++;
          }
          data_buffer->current_pos++;
        }

        header[m] = '\0';
        if (m == 0) {
          empty_line = 1;
        } else {
          data_buffer->current_pos++;
          debug("< %s", header);
          if (x == 0) { // first header received
            parse_response(header);
            if (headers_info.status_code == 404) {
              die("Not found.");
            } else if (headers_info.status_code == 426) {
              die("Please upgrade MSXHub to the latest version to continue.");
            } else if (headers_info.status_code >= 300 && headers_info.status_code < 400) {
              headers_info.is_redirect = 1;
            } else if (headers_info.status_code != 200) {
              die ("HTTP Error: %d", headers_info.status_code);
            }
          } else {
            parse_header(header, title, value);
            tolower_str(title);
            if (strcmp(title, "content-length") == 0) {
              headers_info.content_length = (unsigned long)atol(value);
            } else if (strcmp(title, "location") == 0) {
              strcpy(headers_info.location, value);
            } else if (strcmp(title, "transfer-encoding") == 0) {
              if (strcmp(value, "chunked") == 0) {
                debug("Transfer encoding is chunked.");
                headers_info.is_chunked = 1;
                data_buffer->chunked_data_available = 1;
              }
            }
          }
        }
        x++;
      } while (data_buffer->current_pos < data_buffer->size && empty_line != 1);
    } while (data_buffer->size == TCP_BUFFER_SIZE && empty_line == 0);
  } while (empty_line != 1);

  data_buffer->current_pos++; // Ignore \n

  data_buffer->data_fetched = 0;

  return ERR_OK;
}


char tcp_get_databyte(char *conn, data_buffer_t *data_buffer) {

  while (data_buffer->data_fetched < headers_info.content_length || data_buffer->chunked_data_available == 1) {
    if (data_buffer->current_pos < data_buffer->size) { // still have data in the buffer
      data_buffer->data_fetched++;
      data_buffer->current_pos++;
      return data_buffer->data[data_buffer->current_pos-1];
    } else { // There is no more data in the buffer. Needs to be fetched
      run_or_die(tcp_get(conn, data_buffer));
      data_buffer->current_pos = 0;
    }
  }

  data_buffer->no_more_data = 1;

  return '\0';
}

char http_get_databyte(char *conn, data_buffer_t *data_buffer) {
  char buffer[160];
  int n;

  if (headers_info.is_chunked == 1) { // Content transfer is chunked
    if (data_buffer->chunk_size == 0) { // Get new chunk size
      n = 0;

      do {
        buffer[n] = tcp_get_databyte(conn, data_buffer);
        n ++;
      } while (buffer[n-1] != ';' && buffer[n-1] != '\r');

      buffer[n-1] = '\0';
      data_buffer->chunk_size = hexstr2ul(buffer);

      // Ignore data until last \n
      while ((buffer[0] = tcp_get_databyte(conn, data_buffer)) != '\n') {
        printf("DATA: %d %d\r\n", buffer[0], '\n');
      }
    }

    if (data_buffer->chunk_size == 0) {
      data_buffer->chunked_data_available = 0;
      data_buffer->no_more_data = 1;
    }
    // Get actual data
    data_buffer->chunk_size--;
    return tcp_get_databyte(conn, data_buffer);
  } else {
    return tcp_get_databyte(conn, data_buffer);
  }
}

// accepts the following special names in pathfilename:
// - CON sends output to stdout
// - VAR sends output to var in data with maxdata
//
// note,use a negative value for maxdata when storing to file
char http_get_content(char *conn, char *hostname, char *username, char *password, unsigned int port, char *method, char *path, char *pathfilename, int maxdata, char *data) {
  int fp;
  int n = 0;
  char buffer[TCP_BUFFER_SIZE];
  unsigned long bytes_written = 0;
  char progress_bar_size = 0;
  char *file_name = NULL;
  url parsed_url;

  run_or_die(http_send(conn, hostname, username, password, port, method, path));
  run_or_die(http_get_headers(conn));

  while (headers_info.is_redirect) {
    tcp_close(conn);
    if (n >= 10) {
      die("Too many redirects.");
    }
    n++;
    parse_url(headers_info.location, &parsed_url);
    if (parsed_url.hostname[0] == '\0') { // Relative URL
      run_or_die(http_send(conn, hostname, username, password, port, method, parsed_url.path));
    } else { // Absolute URL
      run_or_die(http_send(conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, method, parsed_url.path));
    }
    run_or_die(http_get_headers(conn));
  }

  // Prepare file handlers
  if (strcmp(pathfilename, "CON") == 0) {
    fp = 1;
  } else if (strcmp(pathfilename, "VAR") == 0) {
    fp = 128; // bigger number than possible
  } else {
    fp = create(pathfilename, O_RDWR, 0x00);
  }

  if (fp < 0) { // Error is in the least significative byte of p
    n = (fp >> 0) & 0xff;
    printf("Error opening file %s: 0x%X\r\n", pathfilename, n);
    explain(buffer, n);
    die("%s", buffer);
  }

  if (fp > 4 && fp < 128) { // If it's a regular file: progress bar
    printf("\33x5"); // Disable cursor
    progress_bar_size = get_screen_size() - 24 - 12;
    file_name = (unsigned char*)parse_pathname(0, pathfilename);
  }


  // Get data loop
  n = 0;
  while (data_buffer->no_more_data == 0 && n != maxdata) {
    if (fp < 128) { // Store result to file
      while (n < TCP_BUFFER_SIZE && data_buffer->no_more_data == 0) {
        buffer[n] = http_get_databyte(conn, data_buffer);
        n++;
      }
      if (data_buffer->no_more_data == 1) {
        n--; // decrease 1 to avoid copyying the null char
      };
      if (fp > 4) { // It's a regular file
        bytes_written += n;
        printf("\r%-12s ", file_name);
        progress_bar(bytes_written, headers_info.content_length, progress_bar_size, "K");
      }
      write(buffer, n, fp);
    } else { // Store result to string
      while (n != maxdata && data_buffer->no_more_data == 0) {
        data[n] = http_get_databyte(conn, data_buffer);
        n++;
      }
    }
    n = 0;
  }

  if (fp > 4 && fp < 128) { // if it's a regular file
    printf("\r\n");
    printf("\33y5"); // Enable cursor
    close(fp);
  }

  run_or_die(tcp_close(conn));
  return ERR_OK;
}


/*** HTTP functions }}} ***/

/*** functions {{{ ***/

void progress_bar(unsigned long current, unsigned long total, char size, char *unit) {
  int n, m;

  putchar('[');
  if (headers_info.is_chunked == 1) {
    for (n=0; n <= size + 1; n++) {
      putchar('-');
    }
  } else {
    m = (int)((float)current / total * size);
    for (n=0; n <= m; n++) {
      putchar('=');
    }
    putchar('>');
    for (n = m; n < size; n++) {
      putchar(' ');
    }
  }
  printf("] %lu%s", current, unit);
  if (headers_info.is_chunked != 1) {
    printf("/%lu%s", total, unit);
  }
  printf("\33K");
}

void debug(const char *s, ...) {
  if (DEBUG != 0) {
    va_list ap;
    va_start(ap, s);
    printf("*** DEBUG: ");
    vprintf(s, ap);
    printf("\r\n");
    va_end(ap);
  }
}

void debug_nocrlf(const char *s, ...) {
  if (DEBUG != 0) {
    va_list ap;
    va_start(ap, s);
    printf("*** DEBUG: ");
    vprintf(s, ap);
    va_end(ap);
  }
}

// https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
void trim(char * s) {
  char * p = s;
  int l = strlen(p);

  while(isspace(p[l - 1])) p[--l] = 0;
  while(* p && isspace(* p)) ++p, --l;

  memmove(s, p, l + 1);
}

void tolower_str(char *str) {
  int i;

  for(i = 0; str[i]; i++){
    str[i] = tolower(str[i]);
  }
}

void toupper_str(char *str) {
  int i;

  for(i = 0; str[i]; i++){
    str[i] = toupper(str[i]);
  }
}

unsigned long hexstr2ul(char *str) {
  int n;
  char c;
  unsigned long result;

  result = 0;
  n = 0;
  c = ' ';
  while (c != '\0') {
    c = tolower(str[n]);
    if(c >= '0' && c <= '9') {
      result = result*16 + c - '0';
    } else if(c >= 'a' && c <= 'f') {
      result = result*16 + c - 'a' + 10;
    }
    n++;
  }

  return result;
}

void abort_if_esc_is_pressed(void) {
  if ((*((byte*)0xFBEC) & 4) == 0) {
    die("Operation cancelled by user.");
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
void init(void) {
  DEBUG = 0;

  // Check MSX-DOS version >= 2
  msxdosver = dosver();
  if(msxdosver < 2) {
    die("This program requires MSX-DOS 2 to run.");
  }

  // Get the full path of the program
  get_env("PROGRAM", hubpath, sizeof(hubpath));
  hubdrive = hubpath[0];
  hubpath[0] = hubdrive;
  hubpath[1] = '\0';
  strcat(hubpath, ":\\HUB");

  strcpy(configpath, hubpath);
  strcat(configpath, "\\CONFIG");
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

  // Env variables take precedence over config files
  get_env(filename, buffer, sizeof(buffer));
  if (buffer[0] != '\0') {
    debug("Read config from variable: %s = %s", filename, buffer);
    return buffer;
  }

  strcpy(buffer, configpath);
  strcat(buffer, "\\");
  strcat(buffer, filename);
  fp = open(buffer, O_RDONLY);

  // Error is in the least significative byte of p
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    printf("Error reading configuration %s: 0x%X\r\n", buffer, n);
    printf("Did you run 'hub configure'?\r\n");
    explain(buffer, n);
    die("%s", buffer);
  }

  n = read(buffer, sizeof(buffer), fp);
  close(fp);
  buffer[n-1] = '\0';
  debug("Read config from file: %s = %s", filename, buffer);
  return buffer;
}

void read_config(void) {
  strcpy(progsdir, get_config("PROGSDIR"));
  strcpy(baseurl, get_config("BASEURL"));
}

/*** helper functions }}} ***/

/*** commands {{{ ***/

void parse_url(char *url, url *parsed_url) {
  char buffer[MAX_URL_SIZE];
  char *pbuffer;
  int pos;

  // Get scheme
  if (string_starts_with(url, "http://")) {
    strcpy(parsed_url->scheme, "http");

    strcpy(buffer, url);
    pbuffer = &buffer[7]; // Ignore http://

    // Get username and password
    if (strchr(pbuffer, '@') == NULL) {
      // URL doesn't have user and password
      strcpy(parsed_url->username, "");
      strcpy(parsed_url->password, "");
    } else {
      // URL has user and password
      strcpy(parsed_url->username, get_str_until(pbuffer, &pos, ":"));
      pbuffer = &pbuffer[pos+1];
      pos = 0;

      strcpy(parsed_url->password, get_str_until(pbuffer, &pos, "@"));
      pbuffer = &pbuffer[pos+1];
      pos = 0;
    }

    // Get hostname
    strcpy(parsed_url->hostname, get_str_until(pbuffer, &pos, ":/?"));
    pbuffer = &pbuffer[pos];
    pos = 0;
    if (pbuffer[0] == ':') {
      &pbuffer++;
      parsed_url->port = atoi(get_str_until(pbuffer, &pos, "/?"));
      pbuffer = &pbuffer[pos];
      pos = 0;
    } else {
      parsed_url->port = 80;
    }
  } else if (string_starts_with(url, "/")) {
    parsed_url->scheme[0] = '\0';
    parsed_url->username[0] = '\0';
    parsed_url->password[0] = '\0';
    parsed_url->hostname[0] = '\0';
    parsed_url->port = 80;

    strcpy(buffer, url);
    pbuffer = &buffer[0];
  } else {
    die("Protocol not supported in URL: %s", url);
  }

  // Get path
  strcpy(parsed_url->path, pbuffer);

  debug("- url scheme: %s", parsed_url->scheme);
  debug("- url username: %s", parsed_url->username);
  debug("- url password: %s", parsed_url->password);
  debug("- url hostname: %s", parsed_url->hostname);
  debug("- url port: %d", parsed_url->port);
  debug("- url path: %s", parsed_url->path);
}

void print_hex(const char *s) {
  while(*s)
    printf("%02x ", (unsigned int) *s++);
  printf("\r\n");
}


char is_installed(char const *package) {
  char path[128];
  file_info_block_t fib;

  // XXX Using DosCall because I wasn't able to implement it myself...
  strcpy(path, configpath);
  strcat(path, "\\IDB\\");
  strcat(path, package);
  regs.Words.DE = (int)&path;
  regs.Bytes.B = 0x00;
  regs.Words.IX = (int)&fib;
  DosCall(FFIRST, &regs, REGS_ALL, REGS_AF);

  if (regs.Bytes.A == 0) {
    return 1;
  } else {
    return 0;
  }
}

void install(char const *package, char const *installdir_arg) {
  char conn = 0;
  char files[MAX_FILES_SIZE];
  char version[16];
  url parsed_url;
  char path[MAX_URLPATH_SIZE];
  char local_path[MAX_PATH_SIZE];
  char created_dirs[MAX_PATH_SIZE*16];
  char installdir[128];
  char *line;
  char *next_line;
  int fp, fp2, n, m, i, l;
  char c;

  if (package[0] == '\0') {
    die("Package name not specified.");
  }

  line = strchr(package, ':');
  if (line == '\0') {
    strcpy(version, "latest");
  } else {
    *line = '\0';
    strcpy(version, ++line);
  }

  if (is_installed(package)) {
    die("Package %s is already installed.", package);
  }

  if (installdir_arg[0] != '\0') {
    if (installdir_arg[1] != ':' && installdir_arg[2] != '\\') {
      die("Installdir must be a full path.");
    }
  }

  read_config();
  init_unapi();
  toupper_str(package);
  parse_url(baseurl, &parsed_url);

  if (installdir_arg[0] == '\0') {
    printf("- Getting installation dir...\r\n");
    strcpy(path, "/api/");
    strcat(path, package);
    strcat(path, "/");
    strcat(path, version);
    strcat(path, "/installdir");
    run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, "VAR", MAX_FILES_SIZE, installdir));
    strcpy(local_path, progsdir);
    strcat(local_path, installdir);
    strcpy(installdir, local_path);
  } else {
    printf("- Installation dir: %s...\r\n", installdir_arg);
    strcpy(installdir, installdir_arg);
  }

  printf("- Creating destination directory: %s\r\n", installdir);
  fp = create(installdir, O_RDWR, ATTR_DIRECTORY);
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    if (n == DIRX) {
      printf("Destination directory already exists.\r\nContinue? (y/N) ");
      c = tolower(getchar());
      printf("\r\n");
      if (c != 'y') {
        die("Aborted");
      }
    } else {
      printf("Error creating destination directory. 0x%X\r\n", n);
      explain(path, n); // Variable path is no longer going to be used. Reusing it as buffer
      die("%s", path);
    }
  }

  printf("- Downloading files...\r\n");
  strcpy(local_path, configpath);
  strcat(local_path, "\\IDB\\");
  strcat(local_path, package);
  fp = create(local_path, O_RDWR, 0x00);

  /*printf("- Getting list of files for package %s:%s...\r\n", package, version);*/

  strcpy(path, "/api/");
  strcat(path, package);
  strcat(path, "/");
  strcat(path, version);
  strcat(path, "/pages");
  run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, "VAR", MAX_FILES_SIZE, files)); // using files as temp variable. It needs to be converted to integer
  m = atoi(files);

  // Iterate pages
  for (n = 0; n<m; n++) {
    debug("Getting files for page %d", n+1);

    strcpy(path, "/api/");
    strcat(path, package);
    strcat(path, "/");
    strcat(path, version);
    strcat(path, "/files/");
    sprintf(files, "%d", n+1); // using variable files as temp variable
    strcat(path, files);
    run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, "VAR", MAX_FILES_SIZE, files));

    line = files;
    created_dirs[0] = '\0';

    // Iterate trough all files
    // https://stackoverflow.com/questions/17983005/c-how-to-read-a-string-line-by-line
    while (line) {
      next_line = strchr(line, '\n');
      if (next_line) *next_line = '\0'; // temporarily terminate the current line

      if (strlen(line) > 0 ) {
        remove_char(line, '\r');

        // Create subdirectories if required
        for (i=0; line[i] != '\0'; i++) {
          if (line[i] == '\\') {
            strcpy(path, installdir);
            strcat(path, "\\");
            c = line[i];
            line[i] = '\0';
            strcat(path, line);
            line[i] = c;
            debug("Creating directory %s", path);

            fp2 = create(path, O_RDWR, ATTR_DIRECTORY);
            if (fp2 < 0) {
              l = (fp2 >> 0) & 0xff;
              if (l != DIRX) {
                printf("Error creating destination directory. 0x%X\r\n", l);
                explain(path, l); // Variable path is no longer going to be used. Reusing it as buffer
                die("%s", path);
              }
            } else {
              if (strstr(created_dirs, path) == '\0') {
                strcat(path, "\n");
                strcat(created_dirs, path);
              }
            }
          }
        }

        strcpy(local_path, installdir);
        strcat(local_path, "\\");
        strcat(local_path, line);

        line = replace_char(line, '\\', '/');
        strcpy(path, "/api/");
        strcat(path, package);
        strcat(path, "/");
        strcat(path, version);
        strcat(path, "/get/");
        strcat(path, package);
        strcat(path, "/");
        strcat(path, line);

        debug("Downloading %s %s to %s\r\n", parsed_url.hostname, path, local_path);
        run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, local_path, -1, NULL));

        strcat(local_path, "\r\n");
        write(local_path, strlen(local_path), fp);
      }

      if (next_line) *next_line = '\n'; // then restore newline-char, just to be tidy
      line = next_line ? (next_line+1) : NULL;
    }
  }

  // After the files, store created directories in IDB
  // TODO iterate in reverse order to allow subdirectories
  line = created_dirs;
  while (line) {
    next_line = strchr(line, '\n');
    if (next_line) *next_line = '\0'; // temporarily terminate the current line

    if (strlen(line) > 0 ) {
      strcpy(local_path, line);
      strcat(local_path, "\r\n");
      write(local_path, strlen(local_path), fp);
    }

    if (next_line) *next_line = '\n'; // then restore newline-char, just to be tidy
    line = next_line ? (next_line+1) : NULL;
  }

  // Save the destination dir to IDB too
  strcpy(local_path, installdir);
  strcat(local_path, "\r\n");
  write(local_path, strlen(local_path), fp);

  close(fp);

  strcpy(local_path, installdir);
  printf("- Done! Package %s installed in %s\r\n", package, local_path);
}

void uninstall(char *package) {
  int fp;
  int n, m;
  char c;
  int bytes_read;
  char buffer[MAX_PATH_SIZE];
  char current_file[MAX_PATH_SIZE];

  if (package[0] == '\0') {
    die("Package name not specified.");
  }

  // Check if installed

  toupper_str(package);

  printf("- Uninstalling package %s...\r\n", package);

  strcpy(buffer, configpath);
  strcat(buffer, "\\IDB\\");
  strcat(buffer, package);
  fp = open(buffer, O_RDONLY);

  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    if (n == 0xD7) {
      die("Package %s is not installed.", package);
    } else {
      printf("Error reading configuration %s: 0x%X\r\n", buffer, n);
      explain(buffer, n);
      die("%s", buffer);
    }
  }

  // Get list of files and deleting them

  current_file[0] = '\0';
  m = 0;
  while(1) {
    bytes_read = read(buffer, MAX_PATH_SIZE, fp);

    for (n=0; n<bytes_read ;n++) {
      if (buffer[n] != '\n' && buffer[n] != '\r') {
        current_file[m] = buffer[n];
        m++;
      } else if (buffer[n] == '\n') {
        current_file[m] = '\0';
        printf("- %s\r\n", current_file);
        c = delete_file(current_file);
        if (c != 0) {
          if (c == 0xD7) { // File does not exist. Continue
            printf("WARNING: File %s does not exist...\r\n", current_file);
          } else if (c == 0xD0) { // Directory not empty. Continue
            printf("WARNING: Directory %s not empty. Not deleting.\r\n", current_file);
          } else { // Another error
            printf("Error deleting file %s: 0x%X\r\n", current_file, c);
            explain(buffer, c);
            die("%s", buffer);
          }
        }
        m = 0;
      }
    }

    if (bytes_read < MAX_PATH_SIZE) {
      break;
    }
  }
  close(fp);

  // Remove file in idb
  strcpy(buffer, configpath);
  strcat(buffer, "\\IDB\\");
  strcat(buffer, package);
  c = delete_file(buffer);
  if (c != 0) {
    printf("Error deleting file %s: 0x%X\r\n", current_file, c);
    explain(buffer, c);
    die("%s", buffer);
  }
}

void upgrade(char *package) {
  uninstall(package);
  install(package, "");
}

void configure(void) {
  char buffer[255];
  char progsdir[255];
  char c;
  int fp, n;

  // TODO configure autoexec.bat

  printf("Welcome to MSX Hub!\r\n\n");
  printf("It looks like it's the first time you run MSX Hub. It's going to be automatically configured.\r\n\n");

  printf("- Main directory: %s\r\n", hubpath);

  // Create main dir if it doesn't exist
  fp = create(hubpath, O_RDWR, ATTR_DIRECTORY);
  // Error is in the least significative byte of fp
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    if (n != DIRX) {
      printf("Error creating main directory: 0x%X\r\n", n);
      explain(buffer, n);
      die("%s", buffer);
    }
  }

  printf("- Configuration directory: %s\r\n", configpath);

  // Create config dir if it doesn't exist
  fp = create(configpath, O_RDWR, ATTR_DIRECTORY);
  // Error is in the least significative byte of fp
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    if (n == DIRX) {
      printf("Configuration directory already exists.\r\nContinue? (y/N) ");
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

  // Create installed dir if it doesn't exist
  strcpy(buffer, configpath);
  strcat(buffer, "\\IDB");
  fp = create(buffer, O_RDWR, ATTR_DIRECTORY);
  // Error is in the least significative byte of fp
  if (fp < 0) {
    n = (fp >> 0) & 0xff;
    if (n != DIRX) {
      printf("Error creating installed directory: 0x%X\r\n", n);
      explain(buffer, n);
      die("%s", buffer);
    }
  }

  progsdir[0] = hubdrive;
  progsdir[1] = ':';
  progsdir[2] = '\0';
  printf("- Programs are going to be installed in %s\r\n", progsdir);
  save_config("PROGSDIR", progsdir);

  save_config("BASEURL", "http://api.msxhub.com/api/");

  printf("Done! MSX Hub configured properly.\r\n");
}

void categories(void) {
  url parsed_url;
  char conn = 0;

  read_config();
  init_unapi();

  parse_url(baseurl, &parsed_url);
  run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", "/api/categories", "CON", -1, NULL));
}

void list(char *category) {
  url parsed_url;
  char conn = 0;
  char path[MAX_URLPATH_SIZE];

  read_config();
  init_unapi();

  strcpy(path, "/api/list");
  if (category[0] != '\0') {
    strcat(path, "?category=");
    strcat(path, category);
  }

  parse_url(baseurl, &parsed_url);
  run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, "CON", -1, NULL));
}

void info(char *package) {
  url parsed_url;
  char conn = 0;
  char path[MAX_URLPATH_SIZE];

  read_config();
  init_unapi();

  strcpy(path, "/api/");
  strcat(path, package);
  strcat(path, "/info");

  parse_url(baseurl, &parsed_url);
  run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, "CON", -1, NULL));
}

void search(char *search_string) {
  url parsed_url;
  char conn = 0;
  char path[100];

  if (search_string[0] == '\0') {
    die("Search string not specified.");
  }

  read_config();
  init_unapi();

  strcpy(path, "/api/search/");
  strcat(path, search_string);

  parse_url(baseurl, &parsed_url);
  run_or_die(http_get_content(&conn, parsed_url.hostname, parsed_url.username, parsed_url.password, parsed_url.port, "GET", path, "CON", -1, NULL));
}


void installed(void) {
  char path[128];
  file_info_block_t fib;

  read_config();

  printf("- List of installed packages:\r\n");

  // XXX Using DosCall because I wasn't able to implement it myself...
  strcpy(path, configpath);
  strcat(path, "\\IDB\\");
  regs.Words.DE = (int)&path;
  regs.Bytes.B = 0x00;
  regs.Words.IX = (int)&fib;
  DosCall(FFIRST, &regs, REGS_ALL, REGS_AF);

  while (regs.Bytes.A == 0) {
    printf("%s\r\n", fib.filename);
    DosCall(FNEXT, &regs, REGS_ALL, REGS_AF);
  }

}

void version() {
  printf("MSXHub version %s\r\n", MSXHUB_VERSION);
}

void usage() {
  printf("Usage: hub <command>\r\n\r\n");
  printf("Available commands:\r\n");
  printf("  categories\r\n");
  printf("  configure\r\n");
  printf("  help\r\n");
  printf("  info\r\n");
  printf("  install\r\n");
  printf("  installed\r\n");
  printf("  list\r\n");
  printf("  search\r\n");
  printf("  uninstall\r\n");
  printf("  upgrade\r\n");
  printf("  version \r\n");
}

void help(char const *command) {
  version();
  if (command[0] == '\0') {
    usage();
    printf("\r\nUse 'hub help COMMAND' for info about a specific command.\r\n");
    printf("\r\nMSXHub is a MSX software installer. It can download and install packaged software from the internet directly into your MSX.\r\n");
    printf("An UNAPI compatible network card and a working internet connection is required.\r\n");

  } else if (strcicmp(command, "install") == 0) {
    printf("Usage: hub install PACKAGE[:VERSION] [INSTALLDIR]\r\n\r\n");
    printf("Install the specified software package from Internet.\r\n\r\n");
    printf("The optional parameter INSTALLDIR specifies the directory where the package is going to be installed. If left blank, the default one is going to be used.\r\n\r\n");
    printf("It's possible to specify the version to be installed appending :VERSION to the package name. By default \"latest\" is ging to be installed.\r\n");

  } else if (strcicmp(command, "uninstall") == 0) {
    printf("Usage: hub uninstall PACKAGE\r\n\r\n");
    printf("Uninstall the specified packaged application from your MSX.\r\n");

  } else if (strcicmp(command, "info") == 0) {
    printf("Usage: hub info PACKAGE\r\n\r\n");
    printf("Get info for the specified package.\r\n");

  } else if (strcicmp(command, "upgrade") == 0) {
    printf("Usage: hub upgrade PACKAGE\r\n\r\n");
    printf("Upgrade specified package to the latest version.\r\n\r\n");
    printf("Note: upgrade actually uninstalls the currently installed package and installs the latest one\r\n");

  } else if (strcicmp(command, "configure") == 0) {
    printf("Usage: hub configure\r\n\r\n");
    printf("Configure the application for first time run.\r\n");

  } else if (strcicmp(command, "categories") == 0) {
    printf("Usage: hub categories\r\n\r\n");
    printf("Show the list of software categories.\r\n");

  } else if (strcicmp(command, "list") == 0) {
    printf("Usage: hub list [CATEGORY]\r\n\r\n");
    printf("Show the list of available packages to be installed.\r\n\r\n");
    printf("If a category name is passed as second parameter, only packages from the specified category will be listed.");

  } else if (strcicmp(command, "search") == 0) {
    printf("Usage: hub search SEARCH_STRING\r\n\r\n");
    printf("Search for SEARCH_STRING in the online packages database.\r\n");

  } else if (strcicmp(command, "installed") == 0) {
    printf("Usage: hub installed\r\n\r\n");
    printf("Show the list of currently installed packages.\r\n");

  } else if (strcicmp(command, "help") == 0) {
    printf("Usage: hub help COMMAND\r\n\r\n");
    printf("Show help about the specified command.");

  } else if (strcicmp(command, "version") == 0) {
    printf("Show current version: ");
    version();
  }
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
        case 'd':
          DEBUG = 1;
          break;
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
    install(commands[1], commands[2]);
  } else if (strcicmp(commands[0], "uninstall") == 0) {
    uninstall(commands[1]);
  } else if (strcicmp(commands[0], "upgrade") == 0) {
    upgrade(commands[1]);
  } else if (strcicmp(commands[0], "configure") == 0) {
    configure();
  } else if (strcicmp(commands[0], "list") == 0) {
    list(commands[1]);
  } else if (strcicmp(commands[0], "info") == 0) {
    info(commands[1]);
  } else if (strcicmp(commands[0], "categories") == 0) {
    categories();
  } else if (strcicmp(commands[0], "search") == 0) {
    search(commands[1]);
  } else if (strcicmp(commands[0], "installed") == 0) {
    installed();
  } else if (strcicmp(commands[0], "help") == 0) {
    help(commands[1]);
  } else if (strcicmp(commands[0], "version") == 0) {
    version();
  } else {
    help("");
  }

  return 0;
}

/*** main }}} ***/
