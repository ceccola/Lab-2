#include "xerrori.h"
#include "thread.h"
#include "operazioni.h"
#include "strutture.h"

#include <stdio.h>    // printf, scanf, fprintf, fopen, fclose, ...
#include <stdlib.h>   // malloc, free, realloc, exit, atoi, ...
#include <stdbool.h>  // tipo bool, true, false
#include <assert.h>   // macro assert()
#include <string.h>   // strlen, strdup, strcmp, strtok, ...
#include <errno.h>    // variabile globale errno (solo quando serve)
#include <limits.h>   // concetto di infinito


void produttore (char *file, pclock *locks, int nThread){
	FILE *f2 = xfopen(file, "r", QUI); //Apertura file per la lettura delle operazioni
	char *buf = NULL; //Usato da getline
	size_t m = 0; //riutilizzate ad ogni iterazione
	while(true){
		//Legge linea dal file salvando il contenuto in buffer
		ssize_t e = getline(&buf, &m, f2); 
		if(e < 0){
			if(buf != NULL){
				free(buf); //Dealloca il buffer usato per contenere la line 
			}
			break; //Esco dal ciclo 
		}
		char *s = strtok(buf, " "); 
		if(s == NULL) continue;
		char c = s[0];
		switch (c){
			case '+': {
				//Aggiungi arco
				operazione op;
				op.desc = '+';
				op.u = atoi(strtok(NULL, " "));
				op.v = atoi(strtok(NULL, " "));
				op.w = atoi(strtok(NULL, " "));
				xsem_wait(&locks->vuoti, QUI); 
				xpthread_mutex_lock(&locks->mutex, QUI);
				locks->buffer[locks->coda] = op; 
				locks->coda = (locks->coda + 1) % 1024;
				xpthread_mutex_unlock(&locks->mutex, QUI);
				xsem_post(&locks->pieni, QUI);
				break;
			}
			case '-': {
				//Elimina arco
				operazione op;
				op.desc = '-';
				op.u = atoi(strtok(NULL, " "));
				op.v = atoi(strtok(NULL, " "));
				op.w = -1;
				xsem_wait(&locks->vuoti, QUI); 
				xpthread_mutex_lock(&locks->mutex, QUI);
				locks->buffer[locks->coda] = op;
				locks->coda = (locks->coda + 1) % 1024;
				xpthread_mutex_unlock(&locks->mutex, QUI);
				xsem_post(&locks->pieni, QUI);
				break;
				
			}
			default: 
				continue; 
		}
	}
	//Una volta terminato il ciclo di lettura, inserisce un'operazione di terminazione per ogni thread
	operazione o;
	o.desc = 't'; //Segno di terminazione
	for(int i = 0; i< nThread; i++){
		
		sem_wait(&locks->vuoti); 
				xpthread_mutex_lock(&locks->mutex, QUI);
				locks->buffer[locks->coda] = o;
				locks->coda = (locks->coda + 1) % 1024;
				xpthread_mutex_unlock(&locks->mutex, QUI);
				xsem_post(&locks->pieni, QUI);
	}
	fclose(f2);
}
void *consumatore (void *args){
	//Cast del puntatore
	argomenti * a = (argomenti *) args;
	pclock *locks = a->locks; 
	grafo *g = a->g;
	while(true){
		//Preleva l'operazione in attesa dal buffer 
		xsem_wait(&locks->pieni, QUI);
		xpthread_mutex_lock(&locks->mutex, QUI);
		operazione op = locks->buffer[locks->testa];
		locks->testa = (locks->testa + 1) % 1024;
		xpthread_mutex_unlock(&locks->mutex, QUI);
		xsem_post(&locks->vuoti, QUI);
		
		switch(op.desc){
			case '+':
				aggiungi_arco(op.u, op.v, op.w, g);
				break;
			case '-':
				cancella_arco(op.u, op.v, g);
				break;
			case 't':
				return NULL;
			default:
				continue;
		}
	}
}