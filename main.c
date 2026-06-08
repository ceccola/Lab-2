#define _GNU_SOURCE   // abilita le estensioni GNU (es. %ms in scanf, asprintf)
#include "xerrori.h"
#include <stdio.h>    // printf, scanf, fprintf, fopen, fclose, ...
#include <stdlib.h>   // malloc, free, realloc, exit, atoi, ...
#include <stdbool.h>  // tipo bool, true, false
#include <assert.h>   // macro assert()
#include <string.h>   // strlen, strdup, strcmp, strtok, ...
#include <errno.h>    // variabile globale errno (solo quando serve)

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
} grafo; 
//buffer null n linee 
int crivello(int n); 
void hash_put(arco **hashTable, arco *a, int hashSize);
arco *hash_get(int u, int v, arco **hashTable, int hashSize);
void hash_remove(int u, int v, arco**hashTable, int hashSize);
void aggiorna_lista(int partenza, arco * a, grafo *g);

int main (int argc, char *argv[]){
	if(argc < 3){
		termina("Utilizzo: msf.out file_grafo file_archi [-t threads] [-H hashsize] [-M nmutex]");
	}
	int nThread = 3, nMutex = 1000; //Inizializzazione dei parametri ai valori standard
	struct grafo g; //Creazione grafo, allocazione statica in quanto è nel main 
	g.hashSize = 100000; //Inizializzazione 
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
				nMutex = atoi(optarg); 
				break;
			default: 
				xtermina("Inserito parametro non previsto nella CLI", QUI);
		}
	}

/*---------------------------------------------PARSING ARCHI DA FILE---------------------------------------------------- */
	FILE *f = xfopen(argv[1], "r", QUI); //Apertura file per la lettura degli archi del grafo 
	arco **archi;
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
				g.vicini = malloc((g.nNodi) * sizeof(elemento*));
				g.cCon = malloc((g.nNodi) * sizeof(int));
				g.gHash = calloc(g.nArchi, sizeof(arco*));
				archi = malloc(g.nArchi * sizeof(arco));
				if((archi == NULL) || (g.gHash == NULL)) {xtermina("Impossibile allocare memoria",QUI);}
				break; 
			case 'a': //Se la linea rappresenta un arco 
				//Ottieni valori
				int u = atoi(strtok(NULL, " "));;
				int v = atoi(strtok(NULL, " "));;
				int w = atoi(strtok(NULL, " "));;
				//Alloca arco e inizializza campi
				arco *a = (arco *) malloc(sizeof(*a));
				a->u = u; 
				a->v = v; 
				a->weight = w; 
				a->msf = false; 
				hash_put(g.gHash, a, g.hashSize); //Inserisce nella hashTable
				//Aggiorna liste di adiacenza di u e v 
				aggiorna_lista(u,a, &g);
				aggiorna_lista(v,a,&g);
				//Aggiungi l'arco all'array di archi per kruskal e aumenta la posizione di inserimento dell'arco
				archi[cArchi] = a;
				cArchi++;
				break; 
			default: 
				continue; 
		}
	}

	
	return 0; 
}


int crivello(int n){ //Ritorna il numero primo più vicino alla dimensione della tabella hash
	//Crivello di Eratostene
	int *sieve = malloc((n+1) * sizeof(bool)); //false = primo 
	assert(sieve != NULL);
	sieve[0] = sieve[1] = true; //0 e 1 non vengono considerati 
	for(int i = 2; i*i <= n;i++){ 
		//per ogni elemento non composto, segna i suoi multipli come composti
		if(!sieve[i]){
			for (int j = i*i; j<= n; j++){
				sieve[j]=true; 
			}
		}
	}

	for(int i = n; i>=0; i--){
		//Scorre al contrario l'array e restituisci il numero più vicino ad n e primo 
		if(!sieve[i]){
			return i;
		}
	}
}

int hash(int u, int v, int hashSize) {
    long key = ((long)u * 2654435761UL )^ ((long)v * 2246822519UL);
    return (int)(abs(key) % hashSize);
}

void hash_put(arco **hashTable, arco *a, int hashSize){
	int index = hash(a->u, a->v, hashSize); //Ottiene l'indice nell'hashtable
	a->next = hashTable[index];
	hashTable[index] = a;
}

arco *hash_get(int u, int v, arco **hashTable, int hashSize){
	if(v<u){
		int t = u;
		u=v;
		v=t;
	}
	int index = hash(u,v,hashSize);	
	arco *a = hashTable[index];
	while(a != NULL){
		if((a->u == u) && (a->v == v)){
			return a;
		} 
		a = a->next;
		
	} 
	return NULL;
}

void hash_remove(int u, int v, arco**hashTable, int hashSize){
	if(v<u){
		int t = u;
		u=v;
		v=t;
	}
	int index = hash(u,v,hashSize);	
	arco* a = hashTable[index];
	arco*f = NULL;
	if(a!= NULL && a->u == u && a->v == v){ //Se l'arco da eliminare è il primo elimina
		f = a;
		hashTable[index] = a->next;
		free(f);
		return;
	}
	while(a != NULL && a->next != NULL){ //Guarda uno in avanti ed elimina 
		if(a->next->u == u && a->next->v == v){
			f=a->next;
			a->next = a->next->next;
			free(f);
			return;
		} else {
			a = a->next;
		}
	}
}

void aggiorna_lista(int partenza, arco * a, grafo *g){
	elemento *ad_u = malloc(sizeof(*ad_u)); //Alloca elemento
	//inizializza parametri
	ad_u->id = partenza == a->u? a->u : a->v; 
	ad_u->w = a->weight;

	elemento **curr = &g->vicini[partenza];  //Puntatore che devi modificare
	while(*curr != NULL && (*curr)->id < ad_u->id){ //Se l'id è già minore di quello da inserire, scorri in avanti
		curr = &(*curr)->next;
	}
	ad_u->next = *curr; //Quando trovi il punto dove inserire, assegna il puntatore come next
	*curr = ad_u; //Modifica puntatore da modificare
}