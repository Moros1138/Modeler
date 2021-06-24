.PHONY: all clean

all:
ifeq ($(OS),Windows_NT)
	make -f Makefile.windows
else
	make -f Makefile.linux
endif

clean:
ifeq ($(OS),Windows_NT)
	make -f Makefile.windows clean
else
	make -f Makefile.linux clean
endif
