NAME=uio_vlink
SRCFILE:=$(wildcard *.c)


obj-m := $(NAME).o
$(NAME)-objs:= uio_vlink_main.o

KVERS=$(shell uname -r)
CURDIR=$(shell pwd)
 
build: kernel_modules

kernel_modules:
	@cp ${VLINK_SDK}/vlink-backend/front-config.h  .
	@cp ${VLINK_SDK}/vlinkinfra/include/vlink_channel.h .
	@cp ${VLINK_SDK}/vlinkinfra/include/my_config.h . 
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
install:
	modprobe uio
	insmod ./$(NAME).ko
uninstall:
	-rmmod $(NAME)
clean:
	@rm -f *.h
	make  -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
test:
	@echo $($(NAME)-objs)
reset:
	make uninstall -C .
	make install -C .
