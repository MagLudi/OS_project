#SHELL = /bin/sh - default
#
#
EXECUTABLE = wordCount
SOURCES    = linkedList.c word-count.c
OBJECTS    = linkedList.o word-count.o
HEADERS    = linkedList.h

CC   	   = gcc
CFLAGS     = -Wall -std=c99 -ansi
CMACROS    = -D DIAGNOSTIC_

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(CMACROS) -o $(EXECUTABLE) $(OBJECTS)
linkedList.o:  linkedList.c $(HEADERS)
	$(CC) $(CFLAGS) $(CMACROS) -c linkedList.c $(HEADERS)
word-count.o:  word-count.c $(HEADERS)
	$(CC) $(CFLAGS) $(CMACROS) -c word-count.c $(HEADERS)

debug:  $(SOURCES)
	$(CC) $(CFLAGS) $(CMACROS) -g -ggdb -c  $(SOURCES)
	$(CC) $(CFLAGS) $(CMACROS) -g -ggdb -o $(EXECUTABLE) $(OBJECTS)
clean:
	rm -f *~
	rm -f *#
	rm -f *.o
	rm -f ./wordCount.exe
neat:
	rm -f *~
	rm -f *#
	rm -f *.o

