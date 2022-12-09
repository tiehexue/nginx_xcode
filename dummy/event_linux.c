//
//  event_linux.c
//  nginx
//
//  Created by Yuan Wang on 2019/12/20.
//  Copyright © 2019 Yuan Wang. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>

#include "one.h"

#define MAX_EVENTS 100

typedef struct {
    int socket_fd;
    int fd;
    off_t size;
    off_t offset;
} epoll_event_data;

void * event_loop(void *s) {
    int server_socket = *((int *)s);
    
    struct epoll_event ev, events[100];
    
    int efd = epoll_create1(0);
    if (!efd) {
        printf("system call epoll_create faild with %d.\n", efd);
        exit(-1);
    }
    
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;
    PANIC(epoll_ctl, efd, EPOLL_CTL_ADD, server_socket, &ev);
    
    while (1) {
        int count = epoll_wait(efd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < count; i++) {
            struct epoll_event current = events[i];
            if (current.data.fd == server_socket) {
                int client_socket = accept(server_socket, NULL, 0);
                
                if (client_socket > 0) {
                    int flags = fcntl(client_socket, F_GETFL, 0);
                    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
                    
                    ev.events = EPOLLIN;
                    ev.data.fd = client_socket;
                    if (epoll_ctl(efd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
                        printf("system call EPOLL_CTL_MOD faild with %d on epoll fd %d and client %d.\n", errno, efd, client_socket);
                    }
                }
            } else if (current.events & EPOLLIN) {
                int client_socket = current.data.fd;
                
                int fd = send_header(client_socket);
                if (fd > 0) {
                    struct stat st;
                    fstat(fd, &st);
                    
                    epoll_event_data *data = (epoll_event_data *)malloc(sizeof(epoll_event_data));
                    data->socket_fd = client_socket;
                    data->fd = fd;
                    data->offset = 0;
                    data->size = st.st_size;
                    
                    ev.events = EPOLLOUT | EPOLLET;
                    ev.data.ptr = data;
                    
                    if (epoll_ctl(efd, EPOLL_CTL_MOD, client_socket, &ev) == -1) {
                        printf("system call EPOLL_CTL_MOD faild with %d on epoll fd %d and client %d.\n", errno, efd, client_socket);
                    }
                }
            } else if (current.events & EPOLLOUT) {
                epoll_event_data *data = (epoll_event_data *)current.data.ptr;
                int client_socket = data->socket_fd;
                int fd = data->fd;
                off_t rest = data->size - data->offset;
                
                off_t sent = sendfile(client_socket, fd, &data->offset, rest);
                
                if (sent == rest || (sent < 0 && errno != EAGAIN)) {
                    if (epoll_ctl(efd, EPOLL_CTL_DEL, client_socket, NULL) == -1) {
                        printf("system call EPOLL_CTL_DEL faild with %d on epoll fd %d and client %d.\n", errno, efd, client_socket);
                    }
                    
                    if (sent > 0) {
                        //printf("finished sending %ld bytes to %d.\n", data->offset, client_socket);
                    } else {
                        printf("system call sendfile %d return %ld with errno %d and error string '%s' to client %d.\n", fd, sent, errno, strerror(errno), client_socket);
                    }
                    
                    free(data);
                    close(client_socket);
                } else {
                    ev.events = EPOLLOUT | EPOLLET;
                    ev.data.ptr = data;
                    if (epoll_ctl(efd, EPOLL_CTL_MOD, client_socket, &ev) == -1) {
                        printf("system call EPOLL_CTL_MOD faild with %d on epoll fd %d and client %d.\n", errno, efd, client_socket);
                    }
                }
            }
        }
    }
    
    return NULL;
}
