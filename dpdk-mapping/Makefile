DLIBS=libdpdk-mapping.so
SRCs=$(wildcard src/*.c)
OBJs=$(patsubst %.c,%.o,$(SRCs))
INCs=-I./include
%.o:%.c
	gcc  $(INCs) -g3 -Wall -fpic -o $@ -c $<

$(DLIBS):$(OBJs)
	gcc -shared -o $(DLIBS) $(OBJs)

clean:
	-rm -f $(OBJs) $(DLIBS)

install:$(DLIBS)
	rm -f /lib64/$(DLIBS)
	cp ./$(DLIBS) /lib64
uninstall:
	-rm -f /lib64/$(DLIBS)
