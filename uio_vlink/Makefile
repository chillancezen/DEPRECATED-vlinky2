NAME=uio_vlink
SRCFILE:=$(wildcard *.c)


obj-m := $(NAME).o
$(NAME)-objs:= uio_vlink_main.o

KVERS=$(shell uname -r)
CURDIR=$(shell pwd)

build: kernel_modules

kernel_modules:
	@rm -f ./front-config.h
	@cp ../vlink-frontend/front-config.h  . 
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
install:
	modprobe uio
	insmod ./$(NAME).ko
uninstall:
	-rmmod $(NAME)
clean:
	@rm -f ./front-config.h
	make  -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
test:
	@echo $($(NAME)-objs)
reset:
	make uninstall -C .
	make install -C .
