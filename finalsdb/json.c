#include "json.h"
#include <libpq-fe.h>
#include <cjson/cJSON.h>
#include <stdlib.h>

char *pgres_to_json(PGresult *res) {
    cJSON *root = cJSON_CreateObject();
    int num_fields = PQnfields(res);
    int num_rows = PQntuples(res);
    cJSON* body = cJSON_AddArrayToObject(root, "body");

    for (int i = 0; i < num_rows; i++) {
        cJSON *row = cJSON_CreateObject();
        for (int j = 0; j < num_fields; j++) {
            const char *field_name = PQfname(res, j);
            const char *field_value = PQgetvalue(res, i, j);
            cJSON_AddStringToObject(row, field_name, field_value);
        }
        cJSON_AddItemToArray(body, row);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_str;
}