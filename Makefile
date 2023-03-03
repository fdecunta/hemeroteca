# hemeroteca 
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
CC = gcc
FLAGS = -Wall -g
INCLUDES = -lncurses


hemeroteca: main.c filetree.o hemeroteca.h config.h
	$(CC) $(FLAGS) main.c filetree.c -o hemeroteca $(INCLUDES)

filetree.o: filetree.c filetree.h
	$(CC) $(FLAGS) filetree.c -c

install: hemeroteca
	mkdir -p $(PREFIX)/bin
	cp -f hemeroteca $(PREFIX)/bin/
	chmod 755 $(PREFIX)/bin/hemeroteca
	rm hemeroteca
	@echo Hemeroteca se instaló con exito.

uninstall:
	rm -f $(PREFIX)/bin/hemeroteca


.PHONY: install uninstall 
