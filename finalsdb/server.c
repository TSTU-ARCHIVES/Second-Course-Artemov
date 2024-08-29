#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <cjson/cJSON.h>

#include "server.h"
#include "db.h"
#include "json.h"
#include "outquery.h"
#include "inquery.h"

#define HTTP400HEADER "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, Authorization\r\n\r\n"
#define HTTP200HEADER "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type, Authorization\r\n"

char *g200hdr(int len) {
    char *buf = malloc(sizeof(HTTP200HEADER) + 25);
    char intbuf[8];
    sprintf(intbuf, "%d", len);
    strcpy(buf, HTTP200HEADER);
    strcat(buf, "Content-Length: ");
    strcat(buf, intbuf);
    strcat(buf, "\r\n\r\n");
    return buf;
}

struct outquery route(struct inquery *query) {
    if (strcmp(query->method, "GET") == 0) {
        return parse_get(query);
    }
    else if (strcmp(query->method, "POST") == 0) {
        return parse_post(query);
    }
    else if (strcmp(query->method, "DELETE") == 0) {
        return parse_delete(query);
    }
    else if (strcmp(query->method, "OPTIONS") == 0) {
        return parse_options(query);
    }
    else {
        struct outquery res;
        res.headers = HTTP400HEADER;
        res.body = "";
        return res;
    }
}

struct outquery parse_options(struct inquery *query) {
    struct outquery res;
    res.headers = HTTP200HEADER;
    res.body = "";
    return res;
}

struct outquery parse_get(struct inquery *query) {
    char *sql;
    struct outquery res;
    int freesql = 0;
    if (strncmp(query->url, "/data/", 6) == 0) {
        const char *tableName = query->url + 6;  
        sql = malloc(sizeof(tableName) + 40);
        freesql = 1;
        snprintf(sql, sizeof(tableName) + 40, "select * from %s where isdeleted = false;", tableName);
    }
    else if (strncmp(query->url, "/getTables", 10) == 0) {
        sql = "select table_name "
              "from information_schema.tables "
              "where table_schema = 'public';";
    }
    else {
        res.headers = HTTP400HEADER;
        res.body = "{\"error\": \"Unknown URL\"}";
        return res;
    }

    int status;
    PGresult *sql_res = execQuery(sql, &status);

    if (status < 0) {
        res.headers = HTTP400HEADER;
        res.body = "";
    } else {
        char *json = pgres_to_json(sql_res);
        res.headers = g200hdr(strlen(json));
        res.body = json;
    }
    if (freesql) free(sql);
    return res;
}

struct outquery parse_post(struct inquery *query) {
    char *sql;
    struct outquery res;
    cJSON *json;

    if (strncmp(query->url, "/update/", 8) == 0) {
        const char *tableName = query->url + 8;
        json = cJSON_Parse(query->body);
        if (json == NULL) {
            res.headers = HTTP400HEADER;
            res.body = "{\"error\": \"Failed to parse JSON data\"}";
            return res;
        }
        sql = parse_post_update(query, tableName, json);
    }
    else if (strncmp(query->url, "/findById/", 10) == 0) {
        const char *tableName = query->url + 10; // cut 
        json = cJSON_Parse(query->body);
        if (json == NULL) {
            res.headers = HTTP400HEADER;
            res.body = "{\"error\": \"Failed to parse JSON data\"}";
            return res;
        }
        sql = parse_post_find(query, tableName, json);
    }
    else if (strncmp(query->url, "/add/", 5) == 0) {
        const char *tableName = query->url + 5; // cut 
        json = cJSON_Parse(query->body);
        if (json == NULL) {
            res.headers = HTTP400HEADER;
            res.body = "{\"error\": \"Failed to parse JSON data\"}";
            return res;
        }
        sql = parse_post_add(query, tableName, json);
    }
    else {        
        res.headers = HTTP400HEADER;
        res.body = "{\"error\": \"Unknown URL\"}";
        return res;
    }
    
    int status;
    PGresult *sql_res = execQuery(sql, &status);
    printf("sql : %s, status %d\n", sql, status);
    if (status < 0 || sql == NULL) {
        res.headers = HTTP400HEADER;
        res.body = "";
    } else {
        char *outjson = pgres_to_json(sql_res);
        res.headers = g200hdr(strlen(outjson));
        res.body = outjson;
        free(sql);
    }
    cJSON_Delete(json);
    PQclear(sql_res);
    return res;
}

char *parse_post_add(struct inquery *query, const char *tableName, cJSON *json) {
    char *sql = malloc(sizeof(char) * 512); // Должно быть достаточно для начала
    strcpy(sql, "INSERT INTO ");
    strcat(sql, tableName);
    strcat(sql, "(");
    cJSON *field;
    int num = 0;
    cJSON_ArrayForEach(field, json) {
        if (num > 0) {
            strcat(sql, ", ");
        }
        strcat(sql, field->string);
        num++;
    }
    num = 0;
    strcat(sql, ") VALUES( ");
    cJSON_ArrayForEach(field, json) {
        if (num > 0) {
            strcat(sql, ", ");
        }
        strcat(sql, "'");
        strcat(sql, cJSON_GetStringValue(field));
        strcat(sql, "'");
        num++;
    }
    strcat(sql, ")");

    return sql;
}

char *parse_post_update(struct inquery *query, const char *tableName, cJSON *json) {
    char *sql = malloc(sizeof(char) * 512); // Должно быть достаточно для начала
    strcpy(sql, "UPDATE ");
    strcat(sql, tableName);
    strcat(sql, " SET ");
    int num = 0;
    cJSON *field;
    cJSON_ArrayForEach(field, json) {
        if (num > 0) {
            strcat(sql, ", ");
        }
        strcat(sql, field->string);
        strcat(sql, " = '");
        strcat(sql, cJSON_GetStringValue(field));
        strcat(sql, "' ");
        num++;
    }

    int status;
    char *idFieldName = definePrimKey(tableName, &status);
    if (status < 0) {
        free(sql);
        return NULL;
    }
    // Добавляем условие WHERE, если нужно (здесь предполагается что-то универсальное)
    strcat(sql, " WHERE "); // Пример, предполагаем что id передается в JSON
    strcat(sql, idFieldName);
    strcat(sql, " = ");
    strcat(sql, cJSON_GetObjectItem(json, idFieldName)->valuestring);
    strcat(sql, " AND ISDELETED = FALSE");

    return sql;
}

char *parse_post_find(struct inquery *query, const char *tableName, cJSON *json) {
    char *sql = malloc(1024); 
    strcpy(sql, "SELECT * FROM ");
    strcat(sql, tableName);
    int status;
    char *idFieldName = definePrimKey(tableName, &status);
    if (status < 0) {
        free(sql);
        return NULL;
    }
    strcat(sql, " WHERE "); // Пример, предполагаем что id передается в JSON
    strcat(sql, idFieldName);
    strcat(sql, " = ");
    strcat(sql, cJSON_GetObjectItem(json, "id")->valuestring);
    strcat(sql, " AND ISDELETED = FALSE");

    return sql;
}


struct outquery parse_delete(struct inquery *query) {
    char *sql;
    struct outquery res;
    cJSON *json;

    if (strncmp(query->url, "/delete/", 8) == 0) {
        const char *tableName = query->url + 8; // cut 
        json = cJSON_Parse(query->body);
        if (json == NULL) {
            res.headers = HTTP400HEADER;
            res.body = "{\"error\": \"Failed to parse JSON data\"}";
            return res;
        }
        sql = parse_delete_delete(query, tableName, json);
    } 
    else if (strncmp(query->url, "/undoDelete/", 12) == 0) {
        const char *tableName = query->url + 12; // cut 
        json = cJSON_Parse(query->body);
        if (json == NULL) {
            res.headers = HTTP400HEADER;
            res.body = "{\"error\": \"Failed to parse JSON data\"}";
            return res;
        }
        sql = parse_delete_undodelete(query, tableName, json);
    }
    else {
        res.headers = HTTP400HEADER;
        res.body = "{\"error\": \"Unknown URL\"}";
        return res;
    }
    cJSON_Delete(json);
    int status;
    PGresult *sql_res = execQuery(sql, &status);
    if (status < 0 || sql == NULL) {
        res.headers = HTTP400HEADER;
        res.body = "";
    } else {
        res.headers = g200hdr(0);
        res.body = "";
        free(sql);
    }
    PQclear(sql_res);
    return res;
}

char *parse_delete_delete(struct inquery *query, const char *tableName, cJSON *json) {
    char *sql = malloc(1024); 
    strcpy(sql, "UPDATE ");
    strcat(sql, tableName);
    strcat(sql, " SET ISDELETED = TRUE");
    int status;
    char *idFieldName = definePrimKey(tableName, &status);
    if (status < 0) {
        free(sql);
        return NULL;
    }
    strcat(sql, " WHERE "); // Пример, предполагаем что id передается в JSON
    strcat(sql, idFieldName);
    strcat(sql, " = ");
    strcat(sql, cJSON_GetObjectItem(json, "id")->valuestring);
    
    return sql;
}

char *parse_delete_undodelete(struct inquery *query, const char *tableName, cJSON *json) {
    char *sql = malloc(1024); 
    strcpy(sql, "UPDATE ");
    strcat(sql, tableName);
    strcat(sql, " SET ISDELETED = FALSE");
    int status;
    char *idFieldName = definePrimKey(tableName, &status);
    if (status < 0) {
        free(sql);
        return NULL;
    }
    strcat(sql, " WHERE "); // Пример, предполагаем что id передается в JSON
    strcat(sql, idFieldName);
    strcat(sql, " = ");
    strcat(sql, cJSON_GetObjectItem(json, "id")->valuestring);

    return sql;
}

