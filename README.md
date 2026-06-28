## Costruzione di Lu ed Lv in cancella_arco
Dopo che l'arco viene rimosso dalla MSF, si deve determinare se u e v rimangono connessi tramite altri archi della MSF. A questo scopo viene utilizzata la funzione ausiliaria bfs_cc che viene  
chiamata due volte: una per costruire Lu (l'insieme dei nodi raggiungibili da u nella MSF dopo la rimozione) e una per costruire Lv (l'insieme dei nodi raggiungibili da v).
bfs_cc utilizza l'array lu contemporaneamente come coda BFS e come lista dei nodi raggiungibili, servendosi dei due indici read e write che indicano rispettivamente il prossimo nodo 
da visitare e la prossima posizione libera in cui inserire un nodo. Per ogni nodo estratto dalla coda si accede alla sua lista di adiacenza, inserendo in lu i vicini raggiungibili 
tramite archi MSF rimanenti (flag msf == true) non ancora visitati. Per tenere traccia dei nodi già visitati viene usato un array booleano visitati, 
che permette un controllo in O(1) sulla posizione corrispondente all'id del nodo.

## Utilizzo di mutex durante le operazioni concorrenti
Per assicurare l'assenza di race condition nell'esecuzione concorrente delle operazioni di aggiunta e rimozione degli archi vengono utilizzati diversi meccanismi di sincronizzazione:
* Array di mutex sulla hashtable: per l'accesso agli elementi della hashtable viene usato l'array di mutex hash_mux, di dimensione nMutex specificata da linea di comando (o default 1000).
Questo array viene sfruttato per fare lock striping sulla hashtable in modo che un thread che vuole accedere ad un arco all'indice i debba
prima acquisire il mutex di indice i%nMutex. Nel programma le operazioni di lock e unlock sono effettuate in corrispondenza dell'effettiva aggiunta/rimozione
dell'arco dalla hashtable o nel momento in cui si vuole modificare il flag della rappresentazione dell'arco contenuta nella hashtable
* Array di mutex sulle componenti connesse: similmente alla hashtable, l'array di mutex cCon_mux contiene le mutex da acquisire nel momento in cui si vuole agire su una determinata
componente connessa. I mutex sulle componenti connesse di ognuno dei due nodi connessi dall'arco interessato vengono acquisiti prima di effettuare visite e operazioni volte
all'aggiornamento della MSF dopo aver rimosso/aggiunto l'arco stesso,
l'acquisizione avviene in un loop che controlla se un altro thread ha modificato la componente connessa di uno
dei due nodi nel frattempo, in questo caso rilascia entrambe le mutex e ritenta. Per evitare deadlock le mutex vengono acquisite in ordine crescente (e rilasciate
in ordine inverso). Indirettamente queste mutex proteggono vicini, in quanto le operazioni su di esso sono possibili solo dopo aver acquisito i mutex sulle componenti connesse
* rwlock su array cCon: per regolare le letture e scritture dall'array cCon viene usata una rwlock, in modo da permettere letture in parallelo e assicurare che se un thread vuole scrivere nessuno può leggere le informazioni finchè non ha terminato
* Mutex sulle informazioni del grafo: modifiche e accessi ai campi del grafo interessati da rimozioni e aggiunte sono regolate da un mutex per evitare race condition e stati inconsistenti
delle variabili (nArchi, numCoCo, costoMSF). Modifiche e letture di tali campi vengono effettuati dopo la fase di calcolo delle modifiche alla MSF e dopo aver rilasciato il mutex sulle componenti connesse 

### Esempi di operazioni in parallelo
Per un esempio di operazioni parallelizzabili prendiamo un grafo ideale con due componenti connesse {1,2,3} e {4,5,6}, supponiamo che il thread A debba aggiungere l'arco (1,2) e invece 
il thread B debba rimuovere l'arco(5,6), supponiamo che le chiavi della hashtable corrispondenti ai due archi siano diverse. Come prima cosa i thread acquisiranno i mutex sui bucket della 
hashtable, trattandosi di indici diversi entrambe le acquisizioni andranno a buon fine ed entrambi i thread proseguiranno con l'aggiunta o la rimozione dell'arco. Una volta terminate le 
operazioni sulla hash e rilasciato il corrispondente mutex, sia il thread A che il thread B acquisiranno con successo entrambi i mutex relativi alle componenti connesse dei nodi interessati 
dalle operazioni, essendo le componenti diverse. Svolti i dovuti calcoli, i thread rilasceranno i mutex sulle componenti connesse in ordine inverso rispetto all'acquisizione, infine
entrambi dovranno acquisire la lock sulle informazioni del grafo, questo è un passaggio non parallelizzabile quindi uno dei due dovrà attendere che l'altro completi quest'ultima fase.
Nel caso in cui gli indici dei mutex su hashtable siano identici o entrambi i thread debbano operare sulla stessa componente connessa allora le operazioni di accesso e modifica 
di hashtable e componenti connesse verranno svolte da un thread dopo l'altro. 
