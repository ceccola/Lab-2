## Costruzione di Lu ed Lv in cancella_arco
Dopo che l'arco viene rimosso dalla MSF, si deve determinare se u e v rimangono connessi tramite altri archi della MSF. A questo scopo viene utilizzata la funzione ausiliaria bfs_cc che viene  
chiamata due volte: una per costruire Lu (l'insieme dei nodi raggiungibili da u nella MSF dopo la rimozione) e una per costruire Lv (l'insieme dei nodi raggiungibili da v).
bfs_cc utilizza l'array lu contemporaneamente come coda BFS e come lista dei nodi raggiungibili, servendosi dei due indici read e write che indicano rispettivamente il prossimo nodo 
da visitare e la prossima posizione libera in cui inserire un nodo. Per ogni nodo estratto dalla coda si accede alla sua lista di adiacenza, inserendo in lu i vicini raggiungibili 
tramite archi MSF rimanenti (flag msf == true) non ancora visitati. Per tenere traccia dei nodi già visitati viene usato un array booleano visitati, 
che permette un controllo in O(1) sulla posizione corrispondente all'id del nodo.

## Utilizzo di mutex durante le operazioni concorrenti
Per assicurare l'assenza di race condition nell'esecuzione concorrente delle operazioni di aggiunta e rimozione degli archi vengono utilizzati diversi meccanismi di sincronizzazione:
**Array di mutex sulla hashtable**: per l'accesso agli elementi della hashtable viene usato l'array di mutex hash_mux, di dimensione nMutex specificata da linea di comando (o default 1000).
Questo array viene sfruttato per fare lock striping sulla hashtable in modo che un thread che vuole accedere ad un arco all'indice i debba
prima acquisire il mutex di indice i%nMutex. Nel programma le operazioni di lock e unlock sono effettuate in corrispondenza dell'effettiva aggiunta/rimozione
dell'arco dalla hashtable o nel momento in cui si vuole modificare il flag della rappresentazione dell'arco contenuta nella hashtable
**Array di mutex sulle componenti connesse**: similmente alla hashtable, l'array di mutex cCon_mux contiene le mutex da acquisire nel momento in cui si vuole agire su una determinata
componente connessa. I mutex sulle componenti connesse di ognuno dei due nodi connessi dall'arco interessato vengono acquisiti prima di effettuare visite e operazioni volte
all'aggiornamento della MSF dopo aver rimosso/aggiunto l'arco stesso,
l'acquisizione avviene in un loop che controlla se un altro thread ha modificato la componente connessa di uno
dei due nodi nel frattempo, in questo caso rilascia entrambe le mutex e ritenta. Per evitare deadlock le mutex vengono acquisite in ordine crescente (e rilasciate
in ordine inverso). Indirettamente queste mutex proteggono vicini, in quanto le operazioni su di esso sono possibili solo dopo aver acquisito i mutex sulle componenti connesse
**rwlock su array cCon**: per regolare le letture e scritture dall'array cCon viene usata una rwlock, in modo da permettere letture in parallelo e assicurare che se un thread vuole scrivere nessuno può leggere le informazioni finchè non ha terminato
**Mutex sulle informazioni del grafo**: modifiche e accessi ai campi del grafo interessati da rimozioni e aggiunte sono regolate da un mutex per evitare race condition e stati inconsistenti
delle variabili (nArchi, numCoCo, costoMSF). In generale l'aggiornamento di tali variabili avviene mentre sono ancora detenute le mutex sulle componenti connesse, per mantenerlo atomico rispetto alle modifiche di vicini e della hashtable effettuate nello stesso ramo. Fa eccezione il caso di rimozione di un arco MSF che spezza la componente in due (nessun arco ponte trovato): qui le mutex sulle componenti connesse vengono rilasciate prima dell'aggiornamento delle statistiche, poiché la coerenza di cCon in quel caso è già garantita autonomamente dalla rwlock acquisita in scrittura.

### Esempi di operazioni in parallelo
Per un esempio di operazioni parallelizzabili prendiamo un grafo ideale con due componenti connesse {1,2,3} e {4,5,6}, supponiamo che il thread A debba aggiungere l'arco (1,2) e invece 
il thread B debba rimuovere l'arco(5,6), supponiamo che le chiavi della hashtable corrispondenti ai due archi siano diverse. 

THREAD A
1. Calcola l'indice del mutex della hashtable in cui deve essere inserito il nuovo arco e lo acquisisce 
2. Alloca e inizializza i nuovo arco, tentando di inserirlo nella hashtable
3. Rilascia la lock sul bucket della hashtable
4. Aggiorna le liste di adiacenza dei due nodi 
5. Visto che i nodi sono nella stessa componente connessa, esegue una dfs per trovare l'arco di costo massimo nella componente connessa e lo confronta con l'arco inserito
6. Se l'arco trovato ha peso maggiore dell'arco inserito, l'arco trovato dalla dfs va rimosso dalla MSF e l'arco "nuovo" va inserito in essa, il thread prima calcola l'indice del mutex da acquisire per accedere al bucket del vecchio arco, lo acquisisce e setta il suo flag msf a false, poi fa la stessa cosa per il nuovo arco settando il suo flag a true. In seguito aggiorna le liste di adiacenza aggiornando le flag del vecchio e del nuovo arco e infine aggiorna le statistiche del grafo
7. Se invece l'arco da inserire non ha peso maggiore dell'arco maggiore nella componente connessa, viene semplicemnte aumentato il numero di archi nel grafo dopo aver acquisito la mutex sulle statistiche, che viene rilasciata ad operazione completata
8. Vengono rilasciate le mutex sulle componenti connesse 
9. Viene acquisita la lock sulle statistiche, viene stampato il risultato dell'operazione e viene rilasciata la lock sule statistiche

THREAD B

1. Calcola l'indice del mutex da acquisire per accedere al bucket della hashtable in cui c'è l'arco di interesse e vi accede (se l'accesso fallisce, rilascia le lock e termina)
2. Effettua una copia locale dei dati dell'arco e poi sblocca la lock del bucket
3. Aggiorna le liste di adiacenza dei due nodi rimuovendoli reciprocamente dalla lista di vicini
4. Acquisisce di nuovo la mutex sul bucket per rimuovere l'arco da essa, una volta rimosso rilascia il mutex
5. Acquisice il mutex sulle statistiche, diminuisce il numero di archi e rilascia il mutex sulle statistiche 
6. Se l'arco era nella MSF, costruisce Lu e Lv con una bfs sul grafo e cerca l'arco di minor costo che collega un vicino di u con uno di v
	-Se non trova l'arco ponte, si generano due nuove componenti connesse
	1. Acquisisce rwlock in modalità scrittura
	2. Individua in quale delle due componenti connesse è contenuta la vecchia radice
	3. Aggiorna l'altra componente connessa trovandone la radice
	4. Rilscia rwlock e mutex sule componenti connesse
	5. Acquisisce stats lock, aggiorna le statistiche e rilascia stast lock
	-Se trova l'arco ponte
	1. Blocca il mutex sul bucket del candidato
	2. Imposta la flag MSF del candidato a true
	3. Sblocca il mutex sul bucket
	3. Imposta il tag msf a true nelle liste di adiacenza
	4. Aggiorna le statistiche dopo aver acquisito stats mux e lo rilascia una volta terminato
	5. Rilascia le mutex sulle componenti connesse
7. Se l'arco non era nella MSF, rilascia le mutex sulle componenti connesse
8. Stampa i risultati in una sezione critica protetta dal mutex stats mux

