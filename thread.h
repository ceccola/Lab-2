#ifndef THREAD_H
#define THREAD_H
#define _GNU_SOURCE	 // permette di usare estensioni GNU
#define QUI __LINE__, __FILE__
#include "xerrori.h"
#include "operazioni.h"
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
/*---------------------------------------------STRUTTURE------------------------------------*/
typedef struct operazione{
	char desc;
	int u;
	int v; 
	int w;
} operazione;

typedef struct pclock{
	sem_t vuoti, pieni;
	pthread_mutex_t mutex;
	operazione buffer[1024];
	int testa, coda; 
} pclock;

typedef struct argomenti{ //Argomenti per la funzione consumatore
	pclock *locks;
	grafo *g;
} argomenti;

void produttore (char *file, pclock *locks, int nThread);
void *consumatore (void *args);

#endif