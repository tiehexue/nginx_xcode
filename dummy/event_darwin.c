//
//  event_darwin.c
//  nginx_learn
//
//  Created by Yuan Wang on 2019/12/20.
//  Copyright © 2019 Yuan Wang. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <sys/fcntl.h>

#include "one.h"

void * event_loop(void *s) {
    int server_socket = *((int *)s);
    
    int kq = kqueue();

    struct kevent evSet;
    EV_SET(&evSet, server_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
        printf("register kevent failed.\n");
    
    struct kevent evList[32];
    while (1) {
        int events_count = kevent(kq, NULL, 0, evList, 32, NULL);
        for (int i = 0; i < events_count; i++) {
            if ((int)evList[i].ident == server_socket) {
                
                int fd = accept((int)evList[i].ident, NULL, 0);
                
                if (fd > 0) {
                    EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    kevent(kq, &evSet, 1, NULL, 0, NULL);
                    
                    int flags = fcntl(fd, F_GETFL, 0);
                    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
                }
            } else if (evList[i].filter == EVFILT_READ) {
                int client_socket = (int)evList[i].ident;
                
                int fd = send_header(client_socket);
                if (fd > 0) {
                    fd_offset *f_off = (fd_offset *)malloc(sizeof(fd_offset));
                    f_off->fd = fd;
                    f_off->offset = 0;
                    
                    EV_SET(&evSet, client_socket, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void *)f_off);
                    if (kevent(kq, &evSet, 1, NULL, 0, NULL)) {
                        printf("adding kevent failed with errno: %d, parameters: %d %ld (%#.8X).\n", errno, kq, evSet.ident, (int)f_off);
                    }
                }
            } else if (evList[i].filter == EVFILT_WRITE) {
                
                int client_socket = (int)evList[i].ident;
                fd_offset *f_off = (fd_offset *)evList[i].udata;

                if (f_off && f_off->fd > 0) {
                    off_t len = 0;
                    
                    int sent = sendfile(f_off->fd, client_socket, f_off->offset, &len, NULL, 0);
                    
                    if (sent != 0) {
                        if (errno == EAGAIN) {
                            f_off->offset += len;
                            EV_SET(&evSet, client_socket, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, (void *)f_off);
                            kevent(kq, &evSet, 1, NULL, 0, NULL);
                        }
                    } else {
                        // printf("finished sending %ld bytes to %d.\n", (long)(f_off->offset + len), client_socket);
                        free(f_off);
                        close(client_socket);
                    }
                }
            }
        }
    }
    
    return NULL;
}
