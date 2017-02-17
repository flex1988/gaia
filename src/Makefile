all: gaia

CC=gcc

OPTIMIZATION?=-O0
 
CFLAGS=-std=c99 -Wall $(OPTIMIZATION) -D_GNU_SOURCE -g -ggdb

LDFLAGS=-pthread -rdynamic

.PHONY: all clean

OBJ=ga_core.o ga_log.o ga_net.o ga_server.o ga_event.o  ga_connection.o ga_message.o ga_request.o ga_client.o ga_memcache.o ga_response.o ga_mbuf.o ga_util.o ga_array.o ga_forestdb.o ga_string.o ga_signal.o

gaia: $(OBJ)
	gcc -o gaia gaia.c $(OBJ) -pthread -rdynamic -L/usr/local/lib -lforestdb
	
clean:
	@rm *.o gaia
