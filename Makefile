# Danilo Marques Araujo dos Santos - 8598670

OBJ=$(DIR_LIBS)jobs.o $(DIR_LIBS)process.o $(DIR_LIBS)syserror.o $(DIR_LIBS)main.o
DIR_LIBS=./lib/
DIR_BIN=./bin/
CFLAGS=-Wall
BIN=luna
CC=gcc

$(DIR_BIN)$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(DIR_BIN)$(BIN) $(CFLAGS)

jobs.o: $(DIR_LIBS)jobs.c $(DIR_LIBS)syserror.c $(DIR_LIBS)jobs.h $(DIR_LIBS)syserror.h
	$(CC) -c $^ -o $@ $(CFLAGS)
	
process.o: $(DIR_LIBS)jobs.c $(DIR_LIBS)process.c $(DIR_LIBS)jobs.h $(DIR_LIBS)process.h
	$(CC) -c $^ -o $@ $(CFLAGS)
	
syserror.o: $(DIR_LIBS)syserror.c $(DIR_LIBS)syserror.h
	$(CC) -c $^ -o $@ $(CFLAGS)
	
main.o: $(DIR_LIBS)jobs.c $(DIR_LIBS)process.c $(DIR_LIBS)syserror.c $(DIR_LIBS)jobs.h $(DIR_LIBS)process.h $(DIR_LIBS)syserror.h
	$(CC) -c $^ -o $@ $(CFLAGS)
	
clean:
	rm $(OBJ)