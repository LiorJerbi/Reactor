CC = gcc
FLAGS = -Wall -g -fPIC

all: react_server

react_server: st_reactor.so server.c
	$(CC) $(FLAGS) server.c -ldl -o react_server -pthread
	
st_reactor.so: reactor.c reactor.h
	$(CC) $(FLAGS) -shared reactor.c -o libst_reactor.so -pthread


.PHONY: clean all

clean: 
	rm -f *.so *.o react_server