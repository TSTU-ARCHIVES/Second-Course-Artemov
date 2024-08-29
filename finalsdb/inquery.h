#ifndef INQUERY
#define INQUERY

#include <string.h>
#include <stdlib.h>

struct inquery {
  char *method;
  char *url;
  int content_len;
  char *body;
};

int parse_query(char *, struct inquery *);

#endif
