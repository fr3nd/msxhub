/*
 * morestr.c
 * Copyright (C) 2018 Carles Amigó <fr3nd@fr3nd.net>
 *
 */

#include <string.h>
#include <ctype.h>
#include "morestr.h"

// Code extracted from https://github.com/Konamiman/MSX/blob/master/SRC/NETWORK/hget.c
int strncmpi(const char *a1, const char *a2, unsigned size) {
  char c1, c2;
  /* Want both assignments to happen but a 0 in both to quit, so it's | not || */
  while((size > 0) && (c1=*a1) | (c2=*a2)) {
    if (!c1 || !c2 || /* Unneccesary? */
        (islower(c1) ? toupper(c1) : c1) != (islower(c2) ? toupper(c2) : c2))
      return (c1 - c2);
    a1++;
    a2++;
    size--;
  }
  return 0;
}

int string_starts_with(const char* str, const char* start) {
  int len;

  len = strlen(start);
  return strncmpi(str, start, len) == 0;
}

char *get_str_until(char *str, int *p, char *until) {
  char result[128];

  *p = strcspn(str, until);
  strcpy(result, str);
  result[*p] = '\0';

  return result;
}

// https://stackoverflow.com/questions/5457608/how-to-remove-the-character-at-a-given-index-from-a-string-in-c
void remove_char(char *str, char c) {

  char *src, *dst;
  for (src = dst = str; *src != '\0'; src++) {
    *dst = *src;
    if (*dst != c) dst++;
  }
  *dst = '\0';
}
