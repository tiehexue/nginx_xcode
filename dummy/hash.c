//
//  hash.c
//  nginx_learn
//
//  Created by Yuan Wang on 2019/12/18.
//  Copyright © 2019 Yuan Wang. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "one.h"

typedef struct link_node {
    const char *key;
    int fd;
    struct link_node *next;
} link_node;

typedef struct _hash_node {
    int fd;
    union {
        struct link_node *next;
        const char *key;
    };
} hash_node;

typedef struct _hash_map {
    uint size;
    uint count;
    hash_node nodes[INITIAL_SIZE];
} hash_map;

static hash_map cache;

void init_cache() {
    memset(&cache, 0, INITIAL_SIZE * sizeof(hash_node));
    cache.size = INITIAL_SIZE;
}

extern unsigned long crc32_tab[];

static u_long hash(const char *s, u_long len) {
    u_long key = 0;
    for (uint i = 0;  i < len;  i ++) {
        key = crc32_tab[(key ^ s[i]) & 0xff] ^ (key >> 8);
    }
    
    /* Robert Jenkins' 32 bit Mix Function */
    key += (key << 12);
    key ^= (key >> 22);
    key += (key << 4);
    key ^= (key >> 9);
    key += (key << 10);
    key ^= (key >> 2);
    key += (key << 7);
    key ^= (key >> 12);

    /* Knuth's Multiplicative Method */
    key = (key >> 3) * 2654435761;

    return key % INITIAL_SIZE;
}

static int add_to_hash(const char *key, u_long length, u_long index, link_node *head) {
    
    char path[200] = {0};
    size_t len = strlen(html_root);
    if (html_root[len - 1] == '/') {
        len--;
    }
    strncpy(path, html_root, len);
    strncpy(path + len, key, length);
    
    int fd = open(path, O_RDONLY);
    
    if (fd < 1) {
        printf("Path not found %s\n", path);
        return fd;
    }
    
    cache.count ++;
    
    char *key_duplicate = (char *)malloc(sizeof(char) * length + 1);
    strncpy(key_duplicate, key, length);
    key_duplicate[length] = '\0';
    
    if (!head) {
        cache.nodes[index].key = key_duplicate;
        cache.nodes[index].fd = fd;
    
        printf("Added to hash %s : %ld -> %d\n", key, index, fd);
    } else {
        link_node *new = (link_node *)malloc(sizeof(link_node));
        new->key = key_duplicate;
        new->fd = fd;
        new->next = NULL;
        
        head->next = new;
        
        printf("Added to hash %s : %ld -> %d (%#.8X) \n", key, index, fd, (int)head);
    }
    
    return fd;
}

int hash_get(const char *key, u_long length) {
    u_long index = hash(key, length);
    
    if (cache.nodes[index].fd == 0) {
        return add_to_hash(key, length, index, NULL);
    } else if (cache.nodes[index].fd == -1) {
        // conflicts
        link_node *head = cache.nodes[index].next;
        while (head && head->next && strcmp(head->key, key) != 0) {
            head = head->next;
        }
        
        if (strcmp(head->key, key) == 0) {
            return head->fd;
        } else {
            return add_to_hash(key, length, index, head);
        }
    } else if (strcmp(key, cache.nodes[index].key) == 0) {
        return cache.nodes[index].fd;
    } else {
        // conflicts
        link_node *first = (link_node *)malloc(sizeof(link_node));
        first->key = cache.nodes[index].key;
        first->fd = cache.nodes[index].fd;
        first->next = NULL;
        
        cache.nodes[index].fd = -1;
        cache.nodes[index].key = NULL;
        cache.nodes[index].next = first;
        
        return add_to_hash(key, length, index, cache.nodes[index].next);
    }
}
