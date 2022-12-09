//
//  test.c
//  nginx
//
//  Created by dp on 2019/3/20.
//  Copyright © 2019 wy. All rights reserved.
//

#include <stdio.h>

#include "ngx_quick_link.h"

static char * rand_string(char *str, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyz1234567890";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

int main(int argc, const char * argv[]) {

    ngx_quick_link_t *q;
    ngx_quick_link_init(&q);
    
    int i = 0;
    
    clock_t start,end;
    start = clock();
    while (i < 1000000) {
        char buf[33];
        rand_string(buf, 33);
        
        ngx_quick_link_add(q, buf, 32, buf, 1);
        
        ngx_str_t result;
        ngx_quick_link_find(q, buf, 32, &result);
        
        //printf("insert and find, match : %d\n", strcmp(buf, result.data));
        
        i++;
    }
    end = clock();
    printf("100W insert time=%lu\n", (long)end-start);
    
    i = 0;
    ngx_str_t result;
    start = clock();
    while (i < 1000000) {
        char buf[33];
        rand_string(buf, 33);
        
        ngx_quick_link_find(q, buf, 32, &result);
        i++;
    }
    end = clock();
    printf("100W search time=%lu\n",(long)end-start);

    
    return NGX_OK;
}
