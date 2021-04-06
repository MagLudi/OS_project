#SHELL = /bin/sh - default
#
#
EXECUTABLE = shell
TEST       = test
UNIT       = unit
SOURCES    = shell.c cmd.c mem.c utl.c 
OBJECTS    = shell.o cmd.o mem.o utl.o  
HEADERS    = utl.h cmd.h mem.h shell.h

CC   	   = gcc
CFLAGS     = -Wall -std=c99 -ansi

#                                      
# setting env variabe DIAGNOSTIC_
# to set env variable from command line instead: make CMACROS='_D DIAGNOSTIC_'
# 
CMACROS    = -D DIAGNOSTIC_

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(CMACROS) -o $(EXECUTABLE) $(OBJECTS)
$(TEST): test.o mem.o utl.o
	$(CC) $(CFLAGS) $(CMACROS) -o $(TEST) test.o mem.o utl.o
$(UNIT): unit.o mem.o cmd.o utl.o
	$(CC) $(CFLAGS) $(CMACROS) -o $(UNIT) unit.o mem.o cmd.o utl.o
shell.o:  shell.c $(HEADERS)
	$(CC) $(CFLAGS) $(CMACROS) -c shell.c $(HEADERS)
test.o:  test.c utl.h mem.h
	$(CC) $(CFLAGS) $(CMACROS) -c test.c utl.h mem.h
unit.o:  unit.c minunit.h utl.h mem.h cmd.h
	$(CC) $(CFLAGS) $(CMACROS) -c unit.c  utl.h mem.h cmd.h minunit.h
utl.o:  utl.c utl.h
	$(CC) $(CFLAGS) $(CMACROS) -c utl.c $(HEADERS)
cmd.o:  cmd.c cmd.h mem.h utl.h
	$(CC) $(CFLAGS) $(CMACROS) -c cmd.c $(HEADERS)
mem.o:  mem.c mem.h utl.h
	$(CC) $(CFLAGS) $(CMACROS) -c mem.c $(HEADERS)
debug:  $(SOURCES)
	$(CC) $(CFLAGS) $(CMACROS) -g -ggdb -c  $(SOURCES)
	$(CC) $(CFLAGS) $(CMACROS) -g -ggdb -o $(EXECUTABLE) $(OBJECTS)
clean:
	rm -f *~
	rm -f *#
	rm -f *.o
	rm -f ./shell.exe
neat:
	rm -f *~
	rm -f *#
	rm -f *.o

