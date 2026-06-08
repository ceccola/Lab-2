#define _GNU_SOURCE   // abilita le estensioni GNU (es. %ms in scanf, asprintf)
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
	struct arco *next;
} arco; 

typedef struct elemento{ //Elemento della lista di adiacenza 
	int id; //Indice del nodo 
	int w; //Peso 
	bool msf; //true se questo arco appartiene alla MSF
	struct elemento *next; 
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



int main (int argc, char *argv[]){


}