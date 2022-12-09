//
//  ngx_http_mysql_module_impl.h
//  nginx
//
//  Created by wy on 2019/3/15.
//  Copyright © 2019 wy. All rights reserved.
//

#ifndef ngx_http_hello_module_mysql_h
#define ngx_http_hello_module_mysql_h

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_quick_link.h"

#define MYSQL_SPLITS     9
#define MAX_SPLIT_LENGTH 100
#define MYSQL_MAX_LENGTH 128

typedef struct {
    char *host;
    char *port;
    char *user;
    char *password;
    char *database;
    char *table;
    char *value_column;
    char *key_column;
    char *key;

    char parameters[MYSQL_SPLITS][MAX_SPLIT_LENGTH]; // Dynamically?
} ngx_mysql_query_t;

ngx_int_t ngx_mysql_query_init(ngx_mysql_query_t *q, ngx_str_t *query);
ngx_int_t ngx_mysql_query(ngx_str_t *value, ngx_str_t *query, ngx_log_t *log);

#endif /* ngx_http_hello_module_mysql_h */
