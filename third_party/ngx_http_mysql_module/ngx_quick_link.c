//
//  ngx_quick_link.c
//  ngx_http_mysql_module
//
//  Created by wy on 2019/3/18.
//  Copyright © 2019 wy. All rights reserved.
//

#include "ngx_quick_link.h"

#define QUICK_LINK_DEFAULT_LEVEL 4
#define QUICK_LINK_DATA_SIZE 2

ngx_int_t ngx_quick_data_init(ngx_quick_link_t *q_link, ngx_quick_layer_node_t *q_node) {
    q_node->data = malloc(sizeof(ngx_quick_data_t));
    
    q_node->data->keys = malloc(QUICK_LINK_DATA_SIZE * sizeof(char *));
    q_node->data->values = malloc(QUICK_LINK_DATA_SIZE * sizeof(char *));
    
    q_node->data->index = 0;
    q_node->data->size = QUICK_LINK_DATA_SIZE;
    
    return NGX_OK;
}

ngx_int_t ngx_quick_data_add(ngx_quick_link_t *q_link, ngx_quick_data_t *q_data, char *key, int key_len, char *value, int value_len) {
    
    ngx_str_t result;
    int i = ngx_quick_data_find(q_data, key, key_len, &result);
    
    if (i != NGX_ERROR) {
        q_data->values[i] = value;
        return NGX_OK;
    }
    
    if (q_data->index == q_data->size) {
        
        char **new_keys = malloc(q_data->size * 2 * sizeof(char *));
        char **new_values = malloc(q_data->size * 2 * sizeof(char *));
        
        ngx_memcpy(new_keys, q_data->keys, q_data->index * sizeof(char *));
        ngx_memcpy(new_values, q_data->values, q_data->index * sizeof(char *));
        
        ngx_free(q_data->keys);
        ngx_free(q_data->values);
        
        q_data->keys = new_keys;
        q_data->values = new_values;
        q_data->size = 2 * q_data->size;
    }
    
    q_data->keys[q_data->index] = malloc((key_len + 1) * sizeof(char));
    ngx_memcpy(q_data->keys[q_data->index], key, key_len + 1);
    q_data->values[q_data->index] = value;
    
    q_data->index++;
    
    return NGX_OK;
}

ngx_int_t ngx_quick_data_find(ngx_quick_data_t *q_data, char *key, int key_len, ngx_str_t *result) {
    
    u_int i = 0;
    while (i < q_data->index) {
        if (strcmp(q_data->keys[i], key) == 0) {
            result->data = (u_char *)q_data->values[i];
            result->len = strlen(q_data->values[i]);
            
            return i;
        }
        
        i++;
    }
    
    return NGX_ERROR;
}

ngx_int_t ngx_quick_link_init(ngx_quick_link_t **q_link) {
    *q_link = malloc(sizeof(ngx_quick_link_t));
    
    (*q_link)->level = QUICK_LINK_DEFAULT_LEVEL;
    (*q_link)->count = 0;
    (*q_link)->root = malloc(sizeof(ngx_quick_layer_node_t));
    
    return NGX_OK;
}

ngx_int_t ngx_quick_link_add(ngx_quick_link_t *q_link, char *key, int key_len, char *value, int value_len) {
    
    ngx_quick_layer_node_t *ptr = q_link->root;

    int i = 0;
    while (i < QUICK_LINK_DEFAULT_LEVEL) {
        
        if(ptr->key == '\0') {
            ptr->key = key[i];
        } else {
            while (ptr->key != key[i] && ptr->next != NULL && (long)ptr->next != 0x3834343134643961) {
            // while (ptr->key != key[i] && ptr->next != NULL) {
                ptr = ptr->next;
            }
            
            if (ptr->key != key[i] && ptr->next == NULL) {
                ngx_quick_layer_node_t *new_node = malloc(sizeof(ngx_quick_layer_node_t));
                new_node->key = key[i];
                ptr->next = new_node;
                ptr = new_node;
            }
        }
        
        if (i < QUICK_LINK_DEFAULT_LEVEL - 1 && ptr->next_layer == NULL) {
            ngx_quick_layer_node_t *new_layer = malloc(sizeof(ngx_quick_layer_node_t));
            ptr->next_layer = new_layer;
        }
        
        if (i < QUICK_LINK_DEFAULT_LEVEL - 1) ptr = ptr->next_layer;
        
        i++;
    }
    
    if (ptr->data == NULL || (long)ptr->data == 0x6431316663396131) {
    //if (ptr->data == NULL) {
        ngx_quick_data_init(q_link, ptr);
    }
    
    ngx_quick_data_add(q_link, ptr->data, key + QUICK_LINK_DEFAULT_LEVEL, key_len - QUICK_LINK_DEFAULT_LEVEL, value, value_len);
    
    q_link->count++;
    
    return NGX_ERROR;
}

ngx_int_t ngx_quick_link_find(ngx_quick_link_t *q_link, char *key, int key_len, ngx_str_t *result) {
    int i = 0;
    
    ngx_quick_layer_node_t *ptr = q_link->root;
    while (i < QUICK_LINK_DEFAULT_LEVEL) {
        while (ptr->key != key[i] && ptr->key != '\0' && ptr->next != NULL) {
            ptr = ptr->next;
        }
        
        if (ptr->key == '\0') {
            return NGX_ERROR;
        }
        
        if (i != QUICK_LINK_DEFAULT_LEVEL - 1) {
            ptr = ptr->next_layer;
        }
        
        i++;
    }
    
    return ngx_quick_data_find(ptr->data, key + QUICK_LINK_DEFAULT_LEVEL, key_len - QUICK_LINK_DEFAULT_LEVEL, result);
}
