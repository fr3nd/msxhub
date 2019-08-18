/*
 * globals.c
 * Copyright (C) 2018 Carles Amigó <fr3nd@fr3nd.net>
 *
 */

#include "globals.h"


// Defining global variables

char DEBUG;
char msxdosver;
char unapiver[90];
char hubdrive;
char hubpath[MAX_PATH_SIZE];
char configpath[MAX_PATH_SIZE];
char progsdir[MAX_PATH_SIZE];
char baseurl[MAX_URL_SIZE];
Z80_registers regs;
unapi_code_block *code_block;
unapi_connection_parameters *parameters;
headers_info_t headers_info;
data_buffer_t *data_buffer;
unsigned long blockSize,currentBlock;
char current_bar_size;