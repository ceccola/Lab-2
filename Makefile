# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-std=c11 -Wall -g -O3 -pthread


# Variabili automatiche: https://www.gnu.org/software/make/manual/make.html#Automatic-Variables
# nei comandi associati ad ogni regola:
#  $@ viene sostituito con il nome del target
#  $< viene sostituito con il primo prerequisito
#  $^ viene sostituito con tutti i prerequisiti
#  % è un jolly che corrisponde a tutte le stringhe non vuote

CC= gcc
CFLAGS= -on msf.out  
DFLAGS= -Wall -Wextra -g

all: main

main: xerrori.c, strutture.c 
	$(CC) $(CFLAGS) $@ $^ $(DFLAGS)

grind: 
	valgrind --leak-check=full --show-leak-kinds=all ./msf.out rome99.gr rome99.mp
 