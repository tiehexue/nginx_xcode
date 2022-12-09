//
//  ngx_quick_link.h
//  ngx_http_mysql_module
//
//  Created by wy on 2019/3/18.
//  Copyright © 2019 wy. All rights reserved.
//

#ifndef ngx_quick_link_h
#define ngx_quick_link_h

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct ngx_quick_data_s {
    char **keys;
    char **values;
    
    size_t index;
    size_t size;
} ngx_quick_data_t;

typedef struct quick_layer_node_s {
    char key;
    struct quick_layer_node_s *next;
    union {
        struct quick_layer_node_s *next_layer;
        ngx_quick_data_t *data;
    };
} ngx_quick_layer_node_t;

typedef struct ngx_quick_link_s {
    int level;
    int count;
    ngx_quick_layer_node_t *root;
} ngx_quick_link_t;

ngx_int_t ngx_quick_data_init(ngx_quick_link_t *q_link, ngx_quick_layer_node_t *q_node);
ngx_int_t ngx_quick_data_add(ngx_quick_link_t *q_link, ngx_quick_data_t *q_data, char *key, int key_len, char *value, int value_len);
ngx_int_t ngx_quick_data_find(ngx_quick_data_t *q_data, char *key, int key_len, ngx_str_t *result);

ngx_int_t ngx_quick_link_init(ngx_quick_link_t **q_link);
ngx_int_t ngx_quick_link_add(ngx_quick_link_t *q_link, char *key, int key_len, char *value, int value_len);
ngx_int_t ngx_quick_link_find(ngx_quick_link_t *q_link, char *key, int key_len, ngx_str_t *result);

ngx_int_t ngx_quick_link_test(ngx_log_t *log);

#endif /* ngx_quick_link_h */
