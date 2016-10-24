include ../vlink.common.mk 
DLIB_TARGET=libvlinkinfra.so
HEADER=./include
CCFLAG+= -g3 -Wall -fpic -O0 
LDFLAG+= -lpthread  -lnuma
CC=gcc
LD=ld
SRC=$(wildcard src/*.c)
SRC+=$(wildcard client/*.c)

OBJ=$(patsubst %.c,%.o,$(SRC))


%.o:%.c
	$(CC) -I$(HEADER)   $(CCFLAG) -c -o $@ $<
$(DLIB_TARGET):$(OBJ)
	@echo VLINK INCLUDE: $(INCLUDE_DIR)
	gcc -shared $(LDFLAG) -o $(DLIB_TARGET)  $(OBJ)
install:$(DLIB_TARGET)
	rm -f /lib64/$(DLIB_TARGET)
	cp ./$(DLIB_TARGET) /lib64
uninstall:
	-rm -f /lib64/$(DLIB_TARGET)
clean:
	-rm -f ./test
	-rm -f $(DLIB_TARGET)
	find -name "*.o" -exec rm -f {} \;
