INCLUDE=-I../include
LIB=-L../


all:
	mkdir -p bin
	gcc -o bin/virt_computer -g3 -lagent $(INCLUDE) $(LIB) virt_computer.c
	gcc -o bin/client_demo -g3 -lagent $(INCLUDE) $(LIB) client_demo.c
clean:
	-rm -rf  ./bin/*
