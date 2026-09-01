#include "xerrori.h"
#include "strutture.h"
#include <stdio.h>    // printf, scanf, fprintf, fopen, fclose, ...
#include <stdlib.h>   // malloc, free, realloc, exit, atoi, ...
#include <stdbool.h>  // tipo bool, true, false
#include <assert.h>   // macro assert()
#include <string.h>   // strlen, strdup, strcmp, strtok, ...
#include <errno.h>    // variabile globale errno (solo quando serve)

int crivello(int n){ //Ritorna il numero primo più vicino alla dimensione della tabella hash
	//Crivello di Eratostene
	bool *sieve = calloc((n+1), sizeof(bool)); //false => il numero è primo 
	if(sieve == NULL) xtermina("Impossibile allocare sieve", QUI);
	sieve[0] = sieve[1] = true; //0 e 1 non vengono considerati 
	for(int i = 2; (long) i*i <= n;i++){ //Scorre fino alla radice quadrata di n
		//per ogni elemento non composto, segna i suoi multipli come composti
		if(!sieve[i]){
			for (int j = i*i; j<= n; j+=i){
				sieve[j]=true; 
			}
		}
	}

	for(int i = n; i>=0; i--){
		//Scorre al contrario l'array e restituisci il numero più vicino ad n e primo 
		if(!sieve[i]){
			free(sieve);
			return i;
		}
	}
	free(sieve);
	return 2;	
}

int hash(int u, int v, int hashSize) {
    long key = ((long)u * 2654435761UL )^ ((long)v * 2246822519UL);
    return (int)(labs(key) % hashSize); //Ritorna il valore assoluto della chiave modulo la dimensione della tabella hash
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
int hash_put(arco *a, grafo *g){ 
	//Normalizzazione nel momento dell'inserimento per robustezza
	int u = a->u, v = a->v;
	if(v < u){ int t=u; u=v; v=t; }
	//Controlla se l'arco è già presente nella hashTable, se sì ritorna 1
	arco *check = hash_get(u, v, g->gHash, g->hashSize);
	if(check != NULL) return 1;
	//Inserisce l'arco in testa alla lista della posizione hash(u,v)
	a->next = g->gHash[hash(u,v,g->hashSize)];
	g->gHash[hash(u,v,g->hashSize)] = a;
	return 0;
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
	//Scorre la lista del bucket e elimina l'arco se lo trova
	while(a != NULL && a->next != NULL){
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

void aggiorna_lista(int u, int v, int w, grafo *g){
	elemento *ad_u = malloc(sizeof(*ad_u)); //Alloca elemento
	//inizializza parametri
	ad_u->id = v; 
	ad_u->w = w;
	ad_u->msf = false;
	elemento **curr = &g->vicini[u];  //Puntatore che deve modificare
	while(*curr != NULL && (*curr)->id < v){ //Se l'id è già minore di quello da inserire, scorre in avanti
		curr = &(*curr)->next;
	}
	ad_u->next = *curr; //Quando trova il punto dove inserire, assegna il puntatore come next
	*curr = ad_u; //Modifica puntatore da modificare
}

bool rimuovi_elemento(int u, int v, elemento **vicini){ //Elimina un elemento dalla lista concatenata vicini
	if(vicini[u] == NULL){
		return false;
	}
	elemento *head = vicini[u];
	
	elemento *prev = NULL;
	while(head != NULL){
		//Se l'id dell'elemento corrente non è quello da eliminare, scorre in avanti
		if(head->id != v){
			prev = head;
			head = head->next;
			continue;
		}
		//Altrimenti elimina l'elemento corrente dalla lista concatenata
		if(prev == NULL){
			vicini[u] = head->next;
			free(head);
			return true;
		} 
		prev->next = head->next;
		free(head);
		return true; 
	}
	return false;
}
void set_msf_flag(elemento **vicini, int u, int v, bool val){ //Imposta a val il flag msf per un arco
	elemento *head = vicini[u];
	while(head != NULL && head->id != v ){
		head = head->next;
	}
	if(head == NULL) xtermina("Vicino non trovato", QUI);
	head->msf = val;
}



/*---------------------------------------------UNION FIND E KRUSKAL-----------------------------------------------------------*/
int find(int *parent, int component){ //Find ricorsiva con path compression
	if(parent[component] == component){
		return component; //Se il componente è la radice di un albero ritornala
	}
	//Altrimenti imposta suo padre = padre di suo padre e ricalcola ricorsivamente
	return parent[component] = find(parent, parent[component]); 
}

void unione(int *parent, int *rank, int n1, int n2){ //Union by rank 
	int r1 = find(parent, n1);
	int r2 = find(parent, n2);
	//Unisce l'albero più piccolo sotto il più grande 
	if(rank[r1] < rank[r2]){
		parent[r1] = r2;
		
	} else if (rank[r1] > rank[r2]){
		parent[r2] = r1;
	} else { 
		parent[r2]=r1; 
		rank[r1]++; //La profondità aumenta di 1 se entrambe sono uguali
	}
}

int compare(const void *a1,const void *a2){ //qsort passa puntatori a void
	//Conversione in puntatori ad arco
	arco *a = *(arco**)a1; 
	arco *b = *(arco**)a2;
	//if per evitare overflow su int dovuto a differenza tra pesi grandi
	if(a->weight < b->weight) return -1;
	else if(a->weight > b->weight) return 1;
	else return 0;
}


long kruskalAlgo(grafo *g, arco **archi){
	g->numCoCo = g->nNodi;
    //Ordina l'array di archi per peso crescente
    qsort(archi, g->nArchi, sizeof(arco*), compare);

    int *parent = malloc(g->nNodi * sizeof(int)); //Array che indica il padre di ogni nodo
	int *rank = malloc(g->nNodi * sizeof(int)); //rango di ogni nodo 
	for(int i=0; i<g->nNodi; i++ ){
		//Inizializza ogni nodo come componente a sè con rank 0
		parent[i] = i;
		rank[i]=0;
		g->cCon[i] = i; //inizializza l'array cCon con tutti i nodi come componenti connesse a sè
	}

    //Costo MSF da ritornare
    long minCost = 0;
    for (int i = 0; i < g->nArchi; i++) {
        int r1 = find(parent, archi[i]->u);
        int r2 = find(parent, archi[i]->v);
        int wt = archi[i]->weight;

        //Se i parent sono diversi appartengono a cc diverse -> unione
        if (r1 != r2) {
            unione(parent, rank, r1, r2);
			g->numCoCo--;
			archi[i]->msf = true; //Aggiorna il flag dell'arco 
			//Aggiorna il tag dell'arco nelle liste di adiacenza 
			set_msf_flag(g->vicini, archi[i]->u, archi[i]->v, true);
			set_msf_flag(g->vicini, archi[i]->v, archi[i]->u, true);
			
            minCost += wt;
        }
    }

	//Popola cCon
	int *min_nodo = malloc(g->nNodi * sizeof(int)); //min_nodo[r] = nodo minore della c.c. con radice r
	for(int i = 0; i < g->nNodi; i++){ //Inizializza min_nodo
		min_nodo[i] = i;
	}
	for(int i = 0; i<g->nNodi; i++){ 
		//Per ogni nodo trova la sua radice e controlla se è il minore delle c.c. (e in caso aggiorna min_nodo)
		int r = find(parent, i);
		if(i < min_nodo[r]) min_nodo[r] = i;
	}
	for(int i = 0; i<g->nNodi; i++){
		//Per ogni nodo, trova la sua radice e assegna a cCon[i] il nodo minore della c.c. con radice r
		g->cCon[i] = min_nodo[find(parent,i)];
	}
	free(parent);
	free(rank);
	free(min_nodo);
    return minCost;
}
/*----------------------------------------------------FREE------------------------------------------------------------*/
void free_elementi(elemento *el){
	while(el != NULL){
		elemento *nx = el->next;
		free(el);
		el = nx;
	}
}
void free_archi(arco *a, int *nArchi, long *costoMSF, int *maxlen, int *totlen){
	int len = 0;
	while(a != NULL){
		len+=1;
		*nArchi+=1;
		if(a->msf) *costoMSF+=a->weight;
		arco *nxt = a->next;
		free(a);
		a = nxt;
	}
	if(len > *maxlen) *maxlen = len;
	*totlen+=len;
}

void free_grafo(grafo *g, arco **archi){
	//Contatori per il calcolo finale 
	
	int nArchi = 0, numCoCo = 0, contapieni = 0, maxlen = 0, totlen = 0;
	long costoMSF = 0;

	//Libera gli archi, li conta e calcola il peso della MSF
	for(int i = 0; i<g->hashSize; i++){ 
		if(g->gHash[i]!= NULL) contapieni+=1;
		free_archi(g->gHash[i], &nArchi, &costoMSF, &maxlen, &totlen);
	}
	free(archi); //Libera array di archi
	free(g->gHash); //Libera gHash
	//Libera gli elementi e calcola il numero di componenti connesse
	int *cc = calloc(g->nNodi, sizeof(int)); //Array per tenere conto delle componenti già viste 

	for(int i = 0; i< g->nNodi; i++){ 
		//Free degli elementi
		int comp = g->cCon[i];
    	if(cc[comp] == 0){ //Se la componente di i non è stata contata, incremnta il contatore e aggiorna cc
        numCoCo++;
        cc[comp] = 1;
    	}
		free_elementi(g->vicini[i]);
	}
	free(cc);
	//Distrugge le mutex sull'hash
	for(int i=0; i<g->nMutex; i++){
		xpthread_mutex_destroy(&g->hash_mux[i], QUI);
	}
	free(g->hash_mux);
	//Distrugge mutex sulle cc
	for(int i=0; i<g->cCon_mux_dim; i++){
		xpthread_mutex_destroy(&g->cCon_mux[i], QUI);
	}
	free(g->cCon_mux);
	free(g->vicini);
	free(g->cCon);
	fprintf(stdout, "Numero posizioni non vuote: %d\n", contapieni);
	fprintf(stdout, "Lunghezza media liste: %f\n", (float) totlen/contapieni);
	fprintf(stdout, "Lunghezza massima liste: %d\n", maxlen);
	fprintf(stdout, "%d %d %ld\n", nArchi, numCoCo, costoMSF);
}