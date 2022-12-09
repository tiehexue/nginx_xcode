CFLAGS=-Wno-pointer-to-int-cast -g

default:	build

clean:
	rm -rf n2

build:
	gcc $(CFLAGS) nginx_learn.c hash.c crc_table.c \
	web.c event_linux.c \
	-o n2 -I/usr/include/kqueue \
	-lkqueue -lpthread

