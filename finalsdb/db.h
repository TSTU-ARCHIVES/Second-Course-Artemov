#ifndef DB
#define DB

#include <libpq-fe.h>
#include <string.h>
#include <stdlib.h>

PGresult *execQuery(char *, int *);
char *definePrimKey(const char *, int *);

#endif
