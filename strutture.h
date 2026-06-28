#ifndef STRUTTURE_H
#define STRUTTURE_H
#define _GNU_SOURCE	 // permette di usare estensioni GNU
#define QUI __LINE__, __FILE__
#include <stdio.h>	 // permette di usare scanf printf etc ...
#include <stdlib.h>	 // conversioni stringa exit() etc ...
#include <stdbool.h> // gestisce tipo bool
#include <assert.h>	 // permette di usare la funzione ass
#include <string.h>	 // funzioni per stringhe
#include <errno.h>	 // richiesto per usare errno
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>	  /* For O_* constants */
#include <pthread.h>
/*---------------------------------------------STRUTTURE------------------------------------*/
typedef struct arco { //arco del grafo non orientato 
	int u,v; //nodi connessi dall'arco, è sempre vero che u<v
	int weight; //peso arbitrario, potrebbe essere negativo
	bool msf; //Vero se l'arco appartiene alla minimum spanning forest
	struct arco *next; //Puntatore al prossimo elemento nella hashtable 
} arco; 

typedef struct elemento{ //Elemento della lista di adiacenza 
	int id; //Indice del nodo 
	int w; //Peso 
	bool msf; //true se questo arco appartiene alla MSF
	struct elemento *next; //Puntatore al prossimo elemento nella lista di adiacenza
} elemento;

typedef struct grafo {
	arco **gHash; //insieme degli archi
	elemento **vicini;  //liste di adiacenza 
	int *cCon; //Tabella delle componenti connesse 
	int numCoCo; 
	long costoMSF;
	int nNodi;
	int nArchi;
	int hashSize;
	int nMutex;
	pthread_mutex_t *hash_mux;
	pthread_mutex_t *cCon_mux;
	pthread_rwlock_t rwlock;
	int cCon_mux_dim;
	pthread_mutex_t stats_mux;
} grafo; 

/*----------------------------------------HASH----------------------------------------------*/
int crivello(int n); 
int hash(int u, int v, int hashSize);
int hash_put(arco *a, grafo *g);
arco *hash_get(int u, int v, arco **hashTable, int hashSize);
void hash_remove(int u, int v, arco**hashTable, int hashSize);
/*---------------------------------LISTE DI ADIACENZA---------------------------------------*/
void aggiorna_lista(int u, int v, int w, grafo *g);
bool rimuovi_elemento(int u, int v, elemento **vicini);

/*---------------------------------UNION FIND E KRUSKAL-------------------------------------*/
int find(int *parent, int component);
void set_msf_flag_true(elemento **vicini, int u, int v);
void set_msf_flag_false(elemento **vicini, int u, int v);
long kruskalAlgo(grafo *g, arco **archi);

/*----------------------------------------FREE----------------------------------------------*/
void free_grafo(grafo *g, arco **archi);
#endif