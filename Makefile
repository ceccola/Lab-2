# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
JAVAC=javac
CC=gcc
CFLAGS=-std=c11 -Wall -g -O3 -pthread

all: compile_C 

compile_C:
	$(CC) $(CFLAGS) -Wextra main.c xerrori.c strutture.c thread.c operazioni.c -o msf.out
Msf.class:
	$(JAVAC) Msf.java 
clean:
	rm -f msf.out *.class