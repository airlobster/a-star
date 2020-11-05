#!/usr/bin/env make -f

#
# Generated by mm
# Time: 2020-11-03 13:44:38
# By: adidegani
#

DATE=$(shell date +%Y%m%d%H%M%S)
AUTHOR=adidegani
BASE=a-star
GCC=gcc -I. -Wall
CFLAGS=-Wall
LFLAGS=

release:	CFLAGS+=-O3
release:	header version link
	@echo "Stripping symbols"
	@strip $(BASE)
	@echo "Purging object files"
	@rm -f *.o

debug:	CFLAGS+=-g -D_DEBUG_
debug:	BASE=a-star_d
debug:	header version link

link:	_version.o a-star.o dlist.o main.o 
	@echo "Linking"
	@$(GCC) $(CFLAGS) $(LFLAGS) -o $(BASE) *.o

_version.o:	_version.c 
	@echo "Compiling _version.c"
	@$(GCC) $(CFLAGS) -c _version.c

a-star.o:	a-star.c dlist.h a-star.h
	@echo "Compiling a-star.c"
	@$(GCC) $(CFLAGS) -c a-star.c

dlist.o:	dlist.c dlist.h
	@echo "Compiling dlist.c"
	@$(GCC) $(CFLAGS) -c dlist.c

main.o:	main.c a-star.h dlist.h
	@echo "Compiling main.c"
	@$(GCC) $(CFLAGS) -c main.c

clean:
	@rm -f *.o $(BASE) $(BASE)_d

version:
	@echo "Generating _version.c and _version.h"
	@echo "const char* appversion() {return \"$(DATE)\";}\nconst char* appauthor() { return \"$(USER)\";}" > _version.c
	@echo "#ifndef _VERSION_H_\n#define _VERSION_H_\nconst char* appversion();\nconst char* appauthor();\n#endif\n" > _version.h

pack:	release
	@echo "Packing binary"
	@zip $(BASE).$(DATE).bin.zip $(BASE)
	@echo "Packing source code"
	@zip $(BASE).$(DATE).source.zip *.c *.h Makefile

install:	release
	@echo "Installing $(BASE)"
	@cp -i $(BASE) /usr/local/bin/.

uninstall:
	@echo "Uninstalling $(BASE)"
	@rm -f /usr/local/bin/$(BASE)

header:
	@echo "Making '$(BASE)' ($(DATE))"
	@echo -------------------------------------------
