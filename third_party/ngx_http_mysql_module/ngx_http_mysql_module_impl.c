//
//  ngx_http_mysql_module_impl.c
//  nginx
//
//  Created by wy on 2019/3/14.
//  Copyright © 2019 wy. All rights reserved.
//

#include <mysql.h>

#include "ngx_http_mysql_module_impl.h"

ngx_quick_link_t *global_cache;

/*
 * query's structure "host:port:user:password:database:table:value_column:key_column:key_value"
 */
ngx_int_t ngx_mysql_query_init(ngx_mysql_query_t *q, ngx_str_t *query) {
    u_char *ptr = query->data;
    
    u_int i = 0;
    do {
        ngx_memzero(q->parameters[i], MAX_SPLIT_LENGTH);
        u_int j = 0;
        while (ptr < (query->data + query->len) && *ptr != ':') {
            q->parameters[i][j++] = *ptr;
            ptr++;
        }
        ptr++;
        i++;
    } while (i < MYSQL_SPLITS);
    
    q->host = q->parameters[0];
    q->port = q->parameters[1];
    q->user = q->parameters[2];
    q->password = q->parameters[3];
    q->database = q->parameters[4];
    q->table = q->parameters[5];
    q->value_column = q->parameters[6];
    q->key_column = q->parameters[7];
    q->key = q->parameters[8];
    
    return NGX_OK;
}

static void ngx_mysql_clean(MYSQL *conn, MYSQL_RES *res) {
    if (res != NULL) mysql_free_result(res);
    mysql_close(conn);
    mysql_library_end();
    mysql_thread_end();
}

ngx_int_t ngx_mysql_query(ngx_str_t *result, ngx_str_t *query, ngx_log_t *log) {

    char    buffer[MYSQL_MAX_LENGTH]; // Dynamically?
    char    quote[MAX_SPLIT_LENGTH];
    
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    
    result->len = 0;
    result->data = NULL;
    
    ngx_mysql_query_t q;
    ngx_mysql_query_init(&q, query);
    
    if (global_cache == NULL) {
        ngx_quick_link_init(&global_cache);
    }
    
    if (ngx_quick_link_find(global_cache, q.key, strlen(q.key), result) == NGX_OK) {
        return NGX_OK;
    }
    
    MYSQL *conn = mysql_init(NULL);
    if (!(conn = mysql_real_connect(conn, (const char *)q.host, q.user,
                                    q.password, (const char *)q.database,
                                    atoi((const char *)q.port), NULL, 0))) {
        ngx_log_error(NGX_LOG_ALERT, log, 0,
                      "couldn't connect to MySQL server: %s", mysql_error(conn));
        ngx_mysql_clean(conn, res);
        return NGX_ERROR;
    }
    
    ngx_memzero(quote, MAX_SPLIT_LENGTH);
    mysql_real_escape_string_quote(conn, quote, q.key, strlen((const char *)q.key), '\'');
    
    const char *format = "select %s from %s where %s='%s'";
    ngx_memzero(buffer, MYSQL_MAX_LENGTH);
    snprintf(buffer, MYSQL_MAX_LENGTH, format, q.value_column, q.table, q.key_column, quote);
    
    if (mysql_query(conn, (const char *)buffer)) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "MySQL query failed: %s", mysql_error(conn));
        ngx_mysql_clean(conn, res);
        return NGX_ERROR;
    }
    
    res = mysql_store_result(conn);
    row = mysql_fetch_row(res);
    
    if (row == NULL) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "MySQL found no value for key: '%s'", quote);
        
        ngx_mysql_clean(conn, res);
        
        return NGX_ERROR;
    } else {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "MySQL value returned '%s'", row[0] ? row[0] : "NULL");
        
        result->len = strlen(row[0]);
        result->data = ngx_calloc(result->len + 1, log);
        ngx_cpystrn(result->data, (u_char *)row[0], result->len + 1);
    }
    
    char *key = ngx_calloc(strlen(q.key) + 1, log);
    ngx_cpystrn((u_char *)key, (u_char *)q.key, strlen(q.key) + 1);
    ngx_quick_link_add(global_cache, key, strlen(q.key), (char *)result->data, result->len);
    
    ngx_mysql_clean(conn, res);
    
    return NGX_OK;
}
