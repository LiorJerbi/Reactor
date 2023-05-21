CC = gcc
FLAGS = -Wall -g -fPIC

all: react_server

react_server: st_reactor.so server.o
	$(CC) $(FLAGS) -o react_server server.c ./libst_reactor.so -pthread
	
st_reactor.so: st_reactor.o
	$(CC) $(FLAGS) -shared -o st_reactor.o libst_reactor.so -pthread

st_reactor.o: reactor.h reactor.c
	$(CC) $(FLAGS) -c reactor.c

server.o: server.c reactor.h
	$(CC) $(FLAGS) -c server.c
.PHONY: clean all

clean: rm -f *.so *.o react_server