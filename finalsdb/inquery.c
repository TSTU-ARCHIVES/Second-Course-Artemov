#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "inquery.h"

int parse_query(char *buf, struct inquery *out) {
    char *method = malloc(sizeof(char) * 7);
    char *url = malloc(sizeof(char) * 128);
    sscanf(buf, "%7s %127s", method, url);
    char* body = strstr(buf, "\r\n\r\n");
    int content_len = strlen(body);

    out->method = method;
    out->url = url;
    out->content_len = content_len;
    out->body = body;

    return 1;
}