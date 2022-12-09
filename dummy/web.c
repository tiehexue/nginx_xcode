//
//  web.c
//  nginx_learn
//
//  Created by Yuan Wang on 2019/12/19.
//  Copyright © 2019 Yuan Wang. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "one.h"

static const char *status_header = "HTTP/1.0 200 OK\nServer: nginx_learn\n";
static const char *html_content_type[] = {
    "Content-Type: text/html;charset=utf-8\n\n",
    "Content-Type: application/javascript;charset=utf-8\n\n",
    "Content-Type: text/css;charset=utf-8\n\n",
    "Content-Type: image/png;charset=utf-8\n\n",
    "Content-Type: image/jpg;charset=utf-8\n\n"
};

static const char * map_uri_to_path(const char *uri, char *path) {
    
    if (uri[5] == ' ') {
        strncpy(path, "/index.html", 11);
    } else {
        char *http = strstr(uri, "HTTP");
        strncpy(path, uri + 4, (int)(http - uri - 5));
    }
    
    const char *postfix = strrchr(path, '.');
    
    if (!strcmp(postfix, ".js")) {
        return html_content_type[1];
    } else if (!strcmp(postfix, ".css")) {
        return html_content_type[2];
    } else if (!strcmp(postfix, ".png")){
        return html_content_type[3];
    } else if (!strcmp(postfix, ".jpg")){
        return html_content_type[4];
    } else {
        return html_content_type[0];
    }
}

int send_header(int client_socket) {
    
    char buf[1024] = {0};
    read(client_socket, buf, 1024);
    buf[1023] = 0;
    
    if (strlen(buf) > 0) {
        char path[MAX_URL_LENGTH] = {0};
        const char *content_type = map_uri_to_path(buf, path);
        
        int fd = hash_get(path, strlen(path));
        if (fd > 0) {
            write(client_socket, status_header, strlen(status_header));
            write(client_socket, content_type, strlen(content_type));
            
            return fd;
        } else {
            char *status = "HTTP/1.0 404 OK\n";
            write(client_socket, status, strlen(status));
            close(client_socket);
            
            return -1;
        }
    } else {
        return -1;
    }
}
