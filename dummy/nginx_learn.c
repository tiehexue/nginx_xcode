//
//  nginx_learn.c
//  nginx
//
//  Created by Yuan Wang on 2019/12/16.
//  Copyright © 2019 Yuan Wang. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "one.h"

#ifdef NGX_DARWIN
#define LISTEN_COUNT 128
#else
#define LISTEN_COUNT 2048
#include <signal.h>
#endif

#define THREAD_COUNT 2

const char *html_root = NULL;

static int create_socket() {
    
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    PANIC(bind, server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    PANIC(listen, server_socket, LISTEN_COUNT);
    
    printf("ready to serve.\n");
    
    return server_socket;
}

int main(int argc, const char * argv[]) {
    
    if (argc < 2) {
        printf("need to specify root dir.\n");
        exit(-1);
    } else {
        html_root = argv[1];
    }
    
    signal(SIGPIPE, SIG_IGN);
    
    int pid = fork();
    
    if (pid == 0) {
        init_cache();
        int server_socket = create_socket();
        printf("child process: %d, parent: %d.\n", getpid(), getppid());
        event_loop(&server_socket);
    } else {
        init_cache();
        int server_socket = create_socket();
        printf("main process: %d, parent: %d.\n", getpid(), getppid());
        event_loop(&server_socket);
    }
    
    return 0;
}
