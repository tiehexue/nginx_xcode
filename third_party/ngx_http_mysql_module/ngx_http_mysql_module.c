//
//  ngx_http_mysql_module.c
//  nginx
//
//  Created by wy on 2019/3/15.
//  Copyright © 2019 wy. All rights reserved.
//

#include "ngx_http_mysql_module_impl.h"

typedef struct ngx_http_mysql_loc_conf_s {
    ngx_array_t* var_lengths;
    ngx_array_t* var_values;
} ngx_http_mysql_loc_conf_t;

static void* ngx_http_mysql_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_mysql_query_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_mysql_get_handler(ngx_http_request_t *r,
                                            ngx_http_variable_value_t *v, uintptr_t data);

static ngx_command_t ngx_http_mysql_commands[] = {
    {
        ngx_string("mysql_set"),
        NGX_HTTP_LOC_CONF | NGX_CONF_TAKE2,
        ngx_http_mysql_query_set,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_mysql_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_mysql_create_loc_conf,
    NULL
};

ngx_module_t ngx_http_mysql_module = {
    NGX_MODULE_V1,
    &ngx_http_mysql_module_ctx,
    ngx_http_mysql_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static void* ngx_http_mysql_create_loc_conf(ngx_conf_t *cf) {
    ngx_http_mysql_loc_conf_t *conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_mysql_loc_conf_t));
    return conf;
}

static char* ngx_http_mysql_query_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_mysql_loc_conf_t *my_conf = (ngx_http_mysql_loc_conf_t *)conf;
    ngx_str_t                 *values = (ngx_str_t *)cf->args->elts;
    ngx_http_variable_t       *v;
    ngx_int_t                  index;
    ngx_http_script_compile_t  sc;
    
    if (values[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &values[1]);
        return NGX_CONF_ERROR;
    }
    values[1].len--;
    values[1].data++;
    
    v = ngx_http_add_variable(cf, &values[1], NGX_HTTP_VAR_CHANGEABLE | NGX_HTTP_VAR_WEAK);
    if (v == NULL) {
        return NGX_CONF_ERROR;
    }
    
    index = ngx_http_get_variable_index(cf, &values[1]);
    if (index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }
    
    if (v->get_handler == NULL) {
        v->get_handler = ngx_http_mysql_get_handler;
        v->data = index;
    }
    
    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));
    
    sc.cf = cf;
    sc.source = &values[2];
    sc.lengths = &my_conf->var_lengths;
    sc.values = &my_conf->var_values;
    sc.variables = ngx_http_script_variables_count(&values[2]);
    sc.complete_lengths = 1;
    sc.complete_values = 1;
    
    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }
    
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_mysql_get_handler(ngx_http_request_t *r,
                                            ngx_http_variable_value_t *v, uintptr_t data) {
    ngx_str_t result;
    ngx_str_t query;
    
    ngx_http_mysql_loc_conf_t *my_conf = ngx_http_get_module_loc_conf(r, ngx_http_mysql_module);
    
    ngx_http_script_run(r, &query, my_conf->var_lengths->elts, 0, my_conf->var_values->elts);
    
    if (ngx_mysql_query(&result, &query, r->connection->log) != NGX_OK) {
        return NGX_ERROR;
    }
    
    v->len = result.len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = result.data;
    
    return NGX_OK;
}
