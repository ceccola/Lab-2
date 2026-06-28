#define _GNU_SOURCE   // abilita le estensioni GNU (es. %ms in scanf, asprintf)
#include "xerrori.h"
#include "operazioni.h"
#include "strutture.h"
#include "thread.h"

#include <stdio.h>    // printf, scanf, fprintf, fopen, fclose, ...
#include <stdlib.h>   // malloc, free, realloc, exit, atoi, ...
#include <stdbool.h>  // tipo bool, true, false
#include <string.h>   // strlen, strdup, strcmp, strtok, ...
#include <pthread.h>
/*--------------------------------------------------MAIN----------------------------------------------------------------------------------*/
int main (int argc, char *argv[]){
	if(argc < 3){
		termina("Utilizzo: msf.out file_grafo file_archi [-t threads] [-H hashsize] [-M nmutex]");
	}
	char *file_grafo = argv[1];
	char *file_op = argv[2];
	//Inizializzazioni
	int nThread = 3;
	struct grafo g; //Creazione grafo, allocazione statica in quanto è nel main 
	g.hashSize = 100003; //Numero primo più vicino a 100000 (dimensione di default)
	g.vicini = NULL; 
	g.gHash = NULL;
	g.cCon = NULL; 
	g.nMutex = 1000;
	int opt; 

	while((opt=getopt(argc, argv, "t:H:M:")) != -1){ 
		//Controlla per i parametri -t, -H e -M specificando che tutti devono essere seguidi da parametri 
		switch (opt){
			case 't': //parametro che indica il numero di thread
				nThread = atoi(optarg);
				break; 
			case 'H': //parametro che indica la dimensione dell'hash 
				g.hashSize = crivello(atoi(optarg)); //Imposta la dimensione della hashtable al numero primo più vicino 
				break; 
			case 'M': //Parametro che indica il numero di mutex
				g.nMutex = atoi(optarg); 
				break;
			default: 
				xtermina("Inserito parametro non previsto nella CLI", QUI);
		}
	}

/*---------------------------------------------PARSING ARCHI DA FILE--------------------------------------------------------------------- */
	FILE *f = xfopen(file_grafo, "r", QUI); //Apertura file per la lettura degli archi del grafo 
	arco **archi = NULL;
	int cArchi = 0;
	char *buffer = NULL; //Usato da getline
	size_t n = 0; //riutilizzate ad ogni iterazione
	while(true){
		//Legge linea dal file salvando il contenuto in buffer
		ssize_t e = getline(&buffer, &n, f); 
		if(e < 0){
			if(buffer != NULL){
				free(buffer); //Dealloca il buffer usato per contenere la line 
			}
			break; //Esco dal ciclo 
		}
		char *s = strtok(buffer, " "); 
		if(s == NULL) continue;
		char c = s[0];
		switch (c){
			case 'c': //Se è un commento, scarta
				continue; 
			case 'p': //Se è la linea di inizio leggi i valori e inizializza/alloca
				strtok(NULL, " "); //scarta la stringa "np"
				g.nNodi = atoi(strtok(NULL, " ")) + 1;
				g.nArchi = atoi(strtok(NULL, " "));
				g.vicini = calloc(g.nNodi, sizeof(elemento*));
				if(g.vicini == NULL) xtermina("Impossibile allocare vicini", QUI);
				g.cCon = malloc((g.nNodi) * sizeof(int));
				if(g.cCon == NULL) xtermina("Impossibile allocare cCon", QUI);
				g.gHash = calloc(g.hashSize, sizeof(arco*));
				if(g.gHash == NULL) xtermina("Impossibile allocare gHash", QUI);
				archi = malloc(g.nArchi * sizeof(arco*));
				if(archi == NULL) xtermina("Impossibile allocare archi", QUI);
				break; 
			case 'a': //Se la linea rappresenta un arco 
				//Se non è stata ancora trovata la linea di inizio per configurare i valori
				if(archi == NULL) xtermina("Linea di configurazione non trovata", QUI);
				//Ottieni valori
				int u = atoi(strtok(NULL, " "));
				int v = atoi(strtok(NULL, " "));
				int w = atoi(strtok(NULL, " "));
				//Alloca arco e inizializza campi
				arco *a = (arco *) malloc(sizeof(*a));
				a->u = u; 
				a->v = v; 
				a->weight = w; 
				a->msf = false; 
				hash_put(a, &g); //Inserisce nella hashTable
				//Aggiorna liste di adiacenza di u e v 
				aggiorna_lista(u,v,w,&g);
				aggiorna_lista(v,u,w,&g);
				//Aggiungi l'arco all'array di archi per kruskal e aumenta la posizione di inserimento dell'arco
				archi[cArchi] = a;
				cArchi++;
				break; 
			default: 
				continue; 
		}
	}
	fclose(f);
/*---------------------------------------------KRUSKAL----------------------------------------------------------------------------------- */
	g.costoMSF = kruskalAlgo(&g, archi);
	fprintf(stderr, "%d %d %ld \n", g.nArchi, g.numCoCo, g.costoMSF);
/*--------------------------------------------OPERAZIONI CONCORRENTI----------------------------------------------------------------------*/
	//Dichiara e inizializza mutex e strutture necessarie alla sincronizzazione
	pclock locks;
	locks.coda = 0;
	locks.testa = 0;
	xsem_init(&locks.pieni, 0, 0, QUI);
	xsem_init(&locks.vuoti, 0, 300, QUI);
	xpthread_mutex_init(&locks.mutex, NULL, QUI);
	argomenti *a = malloc(sizeof(argomenti));
	a->locks = &locks;
	a->g = &g;

	g.hash_mux = malloc(g.nMutex * sizeof(pthread_mutex_t));
	//Inizializza array di mutex per lock striping su hashtable
	for(int i=0; i<g.nMutex; i++){
		xpthread_mutex_init(&g.hash_mux[i], NULL, QUI);
	}
	pthread_mutexattr_t att;
	pthread_mutexattr_init(&att);
	pthread_mutexattr_settype(&att, PTHREAD_MUTEX_RECURSIVE);
	//Inizializza array di mutex per componenti
	g.cCon_mux_dim= g.nMutex < g.nNodi ? g.nMutex : g.nNodi;
	g.cCon_mux = malloc(g.cCon_mux_dim * sizeof(pthread_mutex_t));
	for(int i=0; i<g.cCon_mux_dim; i++){
		xpthread_mutex_init(&g.cCon_mux[i], &att, QUI);
	}
	pthread_mutexattr_destroy(&att);
	xpthread_rwlock_init(&g.rwlock, NULL, QUI);
	//Inizializza il mutex per la modifica delle statistiche del grafo
	xpthread_mutex_init(&g.stats_mux, NULL, QUI);
	pthread_t *thread_ids = malloc(nThread * sizeof(pthread_t));
	for(int i = 0; i<nThread; i++){
		int ret = xpthread_create( &thread_ids[i], NULL, consumatore, (void *)a, QUI);
		if(ret != 0){
			xperror(ret, "Errore creazione thread");
			xtermina("", QUI);
		}
	}
	//Legge dal file delle operazioni e le mette sul buffer condiviso
	produttore(file_op, &locks, nThread);

	//Attendi la terminazione dei thread
	for(int i=0; i<nThread; i++){
		void *retval;
		int ret = xpthread_join(thread_ids[i], &retval,QUI);
		if (ret != 0){
			xtermina("", QUI);
		}
	}
	//Distruggi semafori
	xsem_destroy(&locks.pieni, QUI);
	xsem_destroy(&locks.vuoti, QUI);
	//Libera struttura per arogmenti funzione consumatore
	free(a);
	//Libera l'array di id dei thread
	free(thread_ids);
	fprintf(stdout, "Operazioni terminate\n");

/*-------------------------FREE + STAMPA STATISTICHE---------------------------------------------------------------------------------------*/
	free_grafo(&g, archi); //Libera la memoria del grafo e contemporaneamente stampa le statistiche
	xpthread_mutex_destroy(&locks.mutex, QUI);
	return 0; 
}