#ifndef SERVER
#define SERVER

#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <cjson/cJSON.h>

#include "db.h"
#include "json.h"
#include "outquery.h"
#include "inquery.h"

struct outquery parse_get          (struct inquery *);
char *parse_delete_delete(struct inquery *, const char *, cJSON *);
char *parse_delete_undodelete(struct inquery *, const char *, cJSON *);
struct outquery parse_delete       (struct inquery *);
struct outquery parse_options      (struct inquery *);
char *parse_post_update  (struct inquery *, const char *, cJSON *);
char *parse_post_add  (struct inquery *, const char *, cJSON *);
char *parse_post_find    (struct inquery *, const char *, cJSON *);
struct outquery parse_post         (struct inquery *);
struct outquery route              (struct inquery *);

#endif