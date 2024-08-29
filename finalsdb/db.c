#include "db.h"
#include <libpq-fe.h>
#include <string.h>
#include <stdlib.h>

PGresult *execQuery(char *query, int *status) {
    // но но но
    char connstr[] = "hostaddr=*.*.*.* port=* dbname=* user=* password=*";
    PGconn *connection;
    PGresult *result;
    connection = PQconnectdb(connstr);
    if (PQstatus(connection) == CONNECTION_BAD) {
        PQfinish(connection);
        *status = -1;
        return NULL;
    }


    result = PQexec(connection, "BEGIN TRANSACTION");
    result = PQexec(connection, query);

    if (PQresultStatus(result) != PGRES_COMMAND_OK && PQresultStatus(result) != PGRES_TUPLES_OK) {
        PQexec(connection, "ROLLBACK");
        PQclear(result);
        PQfinish(connection);

        *status = -1;
        return NULL;
    }


    PQexec(connection, "COMMIT");
    
    PQfinish(connection);

    *status = 1;
    return result;
}

char * definePrimKey(const char *tableName, int *status) {
    const char *queryToDefinePrimKey = "SELECT kcu.COLUMN_NAME "
                                        "FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS tc "
                                        "JOIN INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu "
                                        "ON tc.TABLE_SCHEMA = kcu.TABLE_SCHEMA "
                                        "AND tc.TABLE_NAME = kcu.TABLE_NAME " 
                                        "AND tc.CONSTRAINT_NAME = kcu.CONSTRAINT_NAME "
                                        "WHERE tc.CONSTRAINT_TYPE = 'PRIMARY KEY' "
                                        "AND tc.TABLE_NAME = ";
    char *queryToDefinePrimKeyToExec = malloc(512);
    strcpy(queryToDefinePrimKeyToExec, queryToDefinePrimKey);
    strcat(queryToDefinePrimKeyToExec, "'");
    strcat(queryToDefinePrimKeyToExec, tableName);
    strcat(queryToDefinePrimKeyToExec, "';");

    PGresult *res = execQuery(queryToDefinePrimKeyToExec, status);
    free(queryToDefinePrimKeyToExec);
    return PQgetvalue(res, 0, 0);
}