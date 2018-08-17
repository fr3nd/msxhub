/*
 * globals.h
 * Copyright (C) 2018 Carles Amigó <fr3nd@fr3nd.net>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include "asm.h"

#define MSXHUB_VERSION "0.0.1"

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
#define MAX_HEADER_SIZE (512)
#define MAX_URLPATH_SIZE (128)
#define MAX_PATH_SIZE (64)
#define MAX_URL_SIZE (256)

#define MAX_FILES_SIZE (1024)
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

typedef struct {
  int size;
  int current_pos;
  unsigned long data_fetched;
  int chunked_data_available;
  unsigned long chunk_size;
  char no_more_data;
  char data[TCP_BUFFER_SIZE];
} data_buffer_t;

typedef struct {
  unsigned int status_code;
  unsigned long content_length;
  char is_chunked;
} headers_info_t;

// Declaring global variables

extern char DEBUG;
extern char msxdosver;
extern char unapiver[90];
extern char hubdrive;
extern char hubpath[MAX_PATH_SIZE];
extern char configpath[MAX_PATH_SIZE];
extern char progsdir[MAX_PATH_SIZE];
extern char baseurl[MAX_URL_SIZE];
extern Z80_registers regs;
extern unapi_code_block *code_block;
extern unapi_connection_parameters *parameters;
extern headers_info_t headers_info;
extern data_buffer_t *data_buffer;

#endif /* !GLOBALS_H */
