#include "xerrori.h"
#include "operazioni.h"
#include "strutture.h"
#include <stdio.h>    // printf, scanf, fprintf, fopen, fclose, ...
#include <stdlib.h>   // malloc, free, realloc, exit, atoi, ...
#include <stdbool.h>  // tipo bool, true, false
#include <assert.h>   // macro assert()
#include <string.h>   // strlen, strdup, strcmp, strtok, ...
#include <errno.h>    // variabile globale errno (solo quando serve)
#include <limits.h>   // concetto di infinito

reachList bfs_cc(grafo *g, int u){
	int *visitati = calloc(g->nNodi, sizeof(int)); //Bitmap dei nodi visitati
	int *lu = malloc(g->nNodi * sizeof(int)); //Lista di nodi raggiungibili da start

	lu[0] = u;
	visitati[u] = 1; //Setta a 1 il bit visitati di start

	int read = 0; //Da quale indice leggere
	int write = 1;  //Da quale indice scrivere

	while(read < write){
		elemento *head = g->vicini[lu[read]];
		while(head != NULL){
			if(head->msf && visitati[head->id]==0){
				lu[write] = head->id; //Inserisci il nodo nella lista dei raggiungibili
				visitati[head->id] = 1; //Segnalo come visitato
				write++; //Sposta avanti il puntatore a dove scrivere
			}
			head = head->next;
		}
		read++; //Passa al prossimo da leggere 
		
	}
	reachList res;
	res.vicini = lu;
	res.size= read;
	res.visitati = visitati;
	return res;
}
void update_cc(int *elementi,int size, int newR, grafo *g){
	for(int i = 0; i < size; i++){
		g->cCon[elementi[i]] = newR;
	}
}
int find_min(int *array, int size){
	int min = INT_MAX;
	for(int i = 0; i < size; i++){
		if(array[i] < min) min = array[i];
	}
	return min; 
}

bool dfs_max(int curr, int target, int parent, int *maxW, int *maxU, int *maxV,grafo *g){
	if(curr == target){
		return true;
	}
	elemento *next = g->vicini[curr];
	while(next != NULL){
		if(next->msf && next->id != parent){
			bool found = dfs_max(next->id, target, curr, maxW, maxU, maxV, g);
			if(found){
				if(next->w > *maxW){
					*maxW = next->w;
					*maxU = curr;
					*maxV = next->id;
				}
				return true;
			}
		} 
		next = next->next;
	}
	return false; 
}

bool cancella_arco(int u, int v, grafo *g){
	int index = hash(u,v,g->hashSize);
	xpthread_mutex_lock(&g->hash_mux[index % g->nMutex], QUI);
	arco *a = hash_get(u,v, g->gHash, g->hashSize);
	if(a == NULL){
		fprintf(stdout, "- %d %d 0", u, v);
		xperror(1, "Arco non trovato"); 
		return NULL;
	} 
	//Copia locale dei dati dell'arco 
	arco cpy;
	cpy.u = a->u;
	cpy.v = a->v;
	cpy.weight = a->weight;
	cpy.msf = a->msf;
	xpthread_mutex_unlock(&g->hash_mux[index % g->nMutex], QUI);

	//Locking ottimistico con retry loop per fare in modo di acquisire le corrette mutex per le componenti connesse
	int i, j;
	while(true){
		xpthread_rwlock_rdlock(&g->rwlock, QUI);
		i = g->cCon[u] % g->cCon_mux_dim;
		j = g->cCon[v] % g->cCon_mux_dim;
		if(i > j){ //Fai in modo che i e j siano ordinati in maniera crescente
			int tmp = j;
			j = i; 
			i = tmp;
		}
		xpthread_rwlock_unlock(&g->rwlock, QUI);
		xpthread_mutex_lock(&g->cCon_mux[i], QUI);
		xpthread_mutex_lock(&g->cCon_mux[j], QUI);

		int i2 = g->cCon[u] % g->cCon_mux_dim;
		int j2 = g->cCon[v] % g->cCon_mux_dim;
		if (i == i2 && j == j2) break;
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
	}

	//Aggiorna le liste di adiacenza 
	bool r1 = rimuovi_elemento(u,v,g->vicini);
	if(!r1){
		xperror(1, "Elemento non trovato nella l.a.\n"); 
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
		fprintf(stdout, "- %d %d 0", u, v);
		return NULL;
	} 
	bool r2 = rimuovi_elemento(v,u,g->vicini);
	if(!r2){
		xperror(1, " Elemento non trovato nella l.a.\n"); 
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
		fprintf(stdout, "- %d %d 0", u, v);
		return NULL;
	} 
	xpthread_mutex_lock(&g->hash_mux[index % g->nMutex], QUI);
	//Elimina l'arco dalla hash
	hash_remove(u,v, g->gHash, g->hashSize);
	xpthread_mutex_unlock(&g->hash_mux[index % g->nMutex], QUI);

	xpthread_mutex_lock(&g->stats_mux, QUI);
		g->nArchi--;
	xpthread_mutex_unlock(&g->stats_mux, QUI);

	if(cpy.msf){
		//BFS sulle componenti connesse partento dai due nodi 
		reachList lu = bfs_cc(g, u);
		reachList lv = bfs_cc(g, v);

		//Ricerca dell'arco di minor costo che collega un nodo di Lu con un nodo di Lv
		arco minArco; //Arco per salvare il candidato di costo minore che connette u e v 
		minArco.weight = INT_MAX; 
		for(int i = 0; i < lu.size; i++){ //Per tutti i nodi in Lu
			int id = lu.vicini[i];
			elemento *vicino = g->vicini[id];
			while(vicino != NULL){ //Controlla se uno dei suoi vicini è nei visitati di Lv e ha costo minore del candidato attuale
				if(lv.visitati[vicino->id] && vicino->w < minArco.weight){
					minArco.u = id;
					minArco.v = vicino->id;
					minArco.weight = vicino->w;
				}
				vicino = vicino->next;
			}
		}
		if(minArco.weight == INT_MAX){ //Se l'arco non è stato trovato 
			//Controllo quale dei due nodi ha la radice corretta salvata 
			pthread_rwlock_wrlock(&g->rwlock); //Blocca subito anche la write in modo che nessuno possa intervenire tra la lettura e la modifica
			int rad = g->cCon[u]; //Radice della componente da spezzare
			int update_v = false;
			for(int i = 0; i<lu.size; i++){
				if(lu.vicini[i] == rad){
					update_v = true;
				}
			}
			if(update_v){ //Se va aggiornata la cc di v
				int newR = find_min(lv.vicini, lv.size);
				for (int i = 0; i<lv.size; i++){
					g->cCon[lv.vicini[i]] = newR;
				}
			} else { //Se va aggiornata la cc di u
				int newR = find_min(lu.vicini, lu.size);
				for (int i = 0; i<lu.size; i++){
					g->cCon[lu.vicini[i]] = newR;
				}
			}
			pthread_rwlock_unlock(&g->rwlock);
			xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
			xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
			
			xpthread_mutex_lock(&g->stats_mux, QUI);
			g->numCoCo++; //Diventano due comopnenti connesse a sè
			g->costoMSF -= cpy.weight; //Sottrae il costo dell'arco rimosso dal costo della msf
			xpthread_mutex_unlock(&g->stats_mux, QUI);
		} else { 
			xpthread_mutex_lock(&g->stats_mux, QUI);
			//Aggiorna il costo della msf togliendo il peso dell'arco rimosso e aggiungendo il peso del nuovo arco
			g->costoMSF -= cpy.weight;
			g->costoMSF += minArco.weight;
			xpthread_mutex_unlock(&g->stats_mux, QUI);
			xpthread_mutex_lock(&g->hash_mux[(hash(minArco.u, minArco.v, g->hashSize)) % g->nMutex], QUI);
			//Se l'arco è stato trovato imposta il flag msf a true sia nell'hash che in vicini
			arco *update = hash_get(minArco.u, minArco.v, g->gHash, g->hashSize); 
			update->msf = true; 
			xpthread_mutex_unlock(&g->hash_mux[(hash(minArco.u, minArco.v, g->hashSize)) % g->nMutex], QUI);
			flip_msf_flag(g->vicini, minArco.u, minArco.v);
			flip_msf_flag(g->vicini, minArco.v, minArco.u);	
			xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
			xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
		}

		free(lu.vicini);
		free(lu.visitati);
		free(lv.vicini);
		free(lv.visitati);
	} else {
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
	}
	xpthread_mutex_lock(&g->stats_mux, QUI);
	fprintf(stdout, "- %d %d %d %d %ld \n", u,v, g->nArchi, g->numCoCo, g->costoMSF);
	xpthread_mutex_unlock(&g->stats_mux, QUI);
	return NULL; 
}

bool aggiungi_arco(int u, int v, int w, grafo *g){
	//Controlla che l'arco non esiste già, in quel caso termina subito
	int index = hash(u,v,g->hashSize) % g->nMutex;
	xpthread_mutex_lock(&g->hash_mux[index], QUI);
	//Allocazione e inizializzazione dei campi dell'arco
	arco *a = malloc(sizeof(arco));
	if(a == NULL) xtermina("Impossibile allocare memoria per l'arco", QUI);
	a->u = u; 
	a->v = v;
	a->weight = w; 
	a->msf = false;
	int ok = hash_put(a, g);
	if(ok != 0){
		xpthread_mutex_unlock(&g->hash_mux[index], QUI);
		free(a);
		fprintf(stdout, "+ %d %d %d 0", u, v, w);
		return NULL;
	}
	xpthread_mutex_unlock(&g->hash_mux[index], QUI);

	int cu, cv;
	//Aggiorna le liste di adiacenza
	int i, j;
	while(true){
		xpthread_rwlock_rdlock(&g->rwlock, QUI);
		i = g->cCon[u] % g->cCon_mux_dim;
		j = g->cCon[v] % g->cCon_mux_dim;
		if(i > j){ //Fai in modo che i e j siano ordinati in maniera crescente
			int tmp = j;
			j = i; 
			i = tmp;
		}
		xpthread_rwlock_unlock(&g->rwlock, QUI);
		xpthread_mutex_lock(&g->cCon_mux[i], QUI);
		xpthread_mutex_lock(&g->cCon_mux[j], QUI);

		int i2 = g->cCon[u] % g->cCon_mux_dim;
		int j2 = g->cCon[v] % g->cCon_mux_dim;
		cu = g->cCon[u];
		cv = g->cCon[v];
		if (i == i2 && j == j2) break;
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
	}

	aggiorna_lista(u,v,w,g);
	aggiorna_lista(v,u,w,g);

	if(g->cCon[u] != g->cCon[v]){//Se si trovano in due componenti connesse diverse
		xpthread_mutex_lock(&g->hash_mux[index], QUI);
		a->msf = true;
		xpthread_mutex_unlock(&g->hash_mux[index], QUI);
		xpthread_mutex_lock(&g->stats_mux, QUI);
		g->costoMSF += w;
		g->numCoCo--; //Diminuisce il numero di componenti connesse 
		g->nArchi++;
		xpthread_mutex_unlock(&g->stats_mux, QUI);

		int min = cu < cv ? cu : cv; //Trova il minimo tra le due radici
		//Aggiorna la lista di adiacenza di u o v
		if(min == cu){
			reachList lv = bfs_cc(g, v);
			xpthread_rwlock_wrlock(&g->rwlock, QUI);
			update_cc(lv.vicini, lv.size, min, g);
			xpthread_rwlock_unlock(&g->rwlock, QUI);
			free(lv.vicini);
			free(lv.visitati);

		} else {
			reachList lu = bfs_cc(g, u);
			xpthread_rwlock_wrlock(&g->rwlock, QUI);
			update_cc(lu.vicini, lu.size, min, g);
			xpthread_rwlock_unlock(&g->rwlock, QUI);
			free(lu.vicini);
			free(lu.visitati);
		}
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
	} else { //Se i nodi si trovano nella stessa componente connessa 

		//Valori di riferimento per l'eventuale massimo arco da sostituire
		int maxU, maxV, maxW; 
		maxW = -1;
		dfs_max(u, v, -1, &maxW, &maxU, &maxV, g);
		if(w < maxW){ //Se ha trovato un arco di costo maggiore 
			//Imposta a false la flag msf dell'arco nella hash e nelle liste di adiacenza 
			int old_index = hash(maxU, maxV,g->hashSize) % g->nMutex;
			xpthread_mutex_lock(&g->hash_mux[old_index], QUI);
			arco *old = hash_get(maxU, maxV, g->gHash, g->hashSize);
			old->msf = false;
			xpthread_mutex_unlock(&g->hash_mux[old_index], QUI);
			xpthread_mutex_lock(&g->hash_mux[index], QUI);
			a->msf = true;
			xpthread_mutex_unlock(&g->hash_mux[index], QUI);
			//Imposta a false la flag del nuovo arco nelle liste di adiacenza
			flip_msf_flag(g->vicini, maxU, maxV);
			flip_msf_flag(g->vicini, maxV, maxU);
			//Imposta a true la flag del nuovo arco nelle liste di adiacenza
			flip_msf_flag(g->vicini, u, v);
			flip_msf_flag(g->vicini, v, u);

			int dif = maxW - w;
			xpthread_mutex_lock(&g->stats_mux, QUI);
			g->costoMSF -= dif;
			xpthread_mutex_unlock(&g->stats_mux, QUI);
		}
		xpthread_mutex_unlock(&g->cCon_mux[j], QUI);
		xpthread_mutex_unlock(&g->cCon_mux[i], QUI);
		//Incrementa il numero di archi 
		xpthread_mutex_lock(&g->stats_mux, QUI);
		g->nArchi++;
		xpthread_mutex_unlock(&g->stats_mux, QUI);
	}
	xpthread_mutex_lock(&g->stats_mux, QUI);
	fprintf(stdout, "+ %d %d %d %d %ld \n", u,v, g->nArchi, g->numCoCo, g->costoMSF);
	xpthread_mutex_unlock(&g->stats_mux, QUI);
	return NULL;
}