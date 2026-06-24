#ifndef OPERAZIONI_H
#define OPERAZIONI_H
#define _GNU_SOURCE	 // permette di usare estensioni GNU
#define QUI __LINE__, __FILE__
#include "strutture.h"
#include <stdio.h>	 // permette di usare scanf printf etc ...
#include <stdlib.h>	 // conversioni stringa exit() etc ...
#include <stdbool.h> // gestisce tipo bool
#include <assert.h>	 // permette di usare la funzione ass
#include <string.h>	 // funzioni per stringhe
#include <errno.h>	 // richiesto per usare errno
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>	  /* For O_* constants */
#include <pthread.h>

typedef struct reachList{
	int *vicini; 
	int *visitati;
	int size; 
} reachList;
typedef struct terna{
	int nArchi;
	int numCoCo;
	int costoMSF;
}terna;

bool cancella_arco(int u, int v, grafo *g);
bool aggiungi_arco(int u, int v, int w, grafo *g);
#endif