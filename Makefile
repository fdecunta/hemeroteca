# hemeroteca 
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
CC = gcc
FLAGS = -Wall -g
INCLUDES = -lncurses


hemeroteca: main.c filetree.o historial.o
	$(CC) $(FLAGS) main.c filetree.c historial.c -o hemeroteca $(INCLUDES)

filetree.o: filetree.c filetree.h
	$(CC) $(FLAGS) filetree.c -c

historial.o: historial.c historial.h historial.h
	$(CC) $(FLAGS) historial.c -c

install: hemeroteca
	mkdir -p $(PREFIX)/bin
	cp -f hemeroteca $(PREFIX)/bin/
	chmod 755 $(PREFIX)/bin/hemeroteca
	rm hemeroteca
	mkdir -p $(PREFIX)/etc/hemeroteca
	touch $(PREFIX)/etc/hemeroteca/historial
	chmod 666 $(PREFIX)/etc/hemeroteca/historial
	@echo Hemeroteca se instaló con exito.

uninstall:
	rm -f $(PREFIX)/bin/hemeroteca
	rm -rf $(PREFIX)/etc/hemeroteca


.PHONY: install uninstall 
