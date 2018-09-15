/*
 * morestr.h
 * Copyright (C) 2018 Carles Amigó <fr3nd@fr3nd.net>
 *
 */

#ifndef MORESTR_H
#define MORESTR_H

int strncmpi(const char *a1, const char *a2, unsigned size);
int string_starts_with(const char* str, const char* start);
char *get_str_until(char *str, int *p, char *until);

#endif /* !MORESTR_H */
