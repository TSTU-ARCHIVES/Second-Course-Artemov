#ifndef JSON
#define JSON

#include <stdlib.h>
#include <libpq-fe.h>
#include <cjson/cJSON.h>

char *pgres_to_json(PGresult *);

#endif