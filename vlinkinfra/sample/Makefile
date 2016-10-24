
INCLUDE=-I../include
LIB=-L../


all:
	mkdir -p bin
	gcc -o bin/virt_computer -g3 -lvlinkinfra $(INCLUDE) $(LIB) virt_computer.c
	gcc -o bin/client_demo -g3 -lvlinkinfra $(INCLUDE) $(LIB) client_demo.c
	gcc -o bin/dpdk_mapping_demo -g3 -lvlinkinfra $(INCLUDE) $(LIB)  dpdk-mapping-test.c
clean:
	-rm -rf  ./bin/*
