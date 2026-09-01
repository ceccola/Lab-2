import sys
import heapq
class Arco: 
	def __init__(self, u:int, v:int, w:int):
		self.u = u
		self.v = v
		self.w = w
		self.msf = False
	def __lt__(self, altro):
		if not isinstance(altro, Arco):
			return NotImplemented
		return self.w < altro.w
	def __eq__(self, altro):
		return (self.u == altro.u and self.v == altro.v and self.w == altro.w)

nNodi:int = -1 #Numero di nodi nel grafo
nArchi:int = 0 #Numero di archi nel grafo
costoMSF:int = 0 #Costo della MSF
numCoCo:int #Numero di componenti connesse
hash_map:dict = {} #Tabella hash per gli archi 

if(len(sys.argv) < 3): #Controlla che siano stati passati i giusti argomenti
	sys.sdterr.write("Utilizzo: msf.py file_grafo file_archi")
	sys.exit(1)

#File per grafo e archi
file_grafo:str = sys.argv[1]
file_operazioni:str = sys.argv[2] 
archi:list[list[Arco]] = [] #Lista di archi per ricerca msf

#-------------------------------------LETTURA FILE GRAFO--------------------------------------------------
with open(file_grafo, "r") as file:
	for linea in file: 
		campi = linea.strip().split()
		if(not campi):
			continue
		if(campi[0] == "c"): #Ignora i commenti e continua
			continue
		elif(campi[0] == "p"): #Configura il numero di nodi e di archi
			nNodi = int(campi[2]) +1
			archi = [[] for _ in range(nNodi +1)]
			nArchi = int(campi[3])
		elif(campi[0] == "a"): #Inizializza l'arco e i suoi campi e inserisce nella hashtable
			if (nNodi == -1): #Se non è stato ancora letto il numero di nodi, segnala errore
				sys.stderr.write("Errore: formato file grafo non valido")
				sys.exit(1)
			u:int = int(campi[1])
			v:int = int(campi[2])
			w:int = int(campi[3])
			a:Arco = Arco(u, v, w)
			key:int = hash((min(u,v),max(u,v))) #Genera chiave normalizzando gli indici 
			hash_map[key] = a
			archi[a.u].append(a) #Aggiunge l'arco alla lista degli archi uscenti dal nodo u
			rev:Arco = Arco(v, u, w)
			archi[a.v].append(rev) #Aggiunge l'arco alla lista degli archi uscenti dal nodo v

numCoCo = nNodi #Inizializza il numero di componenti connesse al numero di nodi 

#-------------------------------------LETTURA OPERAZIONI---------------------------------------------------
with open(file_operazioni) as file:
	for linea in file:
		campi = linea.strip().split()
		if(not campi):
			continue
		if(campi[0] == "c"):
			continue
		elif(campi[0] == "+"):
			u:int = int(campi[1])
			v:int = int(campi[2])
			w:int = int(campi[3])
			a:Arco = Arco(u,v,w)
			key = hash((min(u,v),max(u,v)))
			if(key in hash_map):
				sys.stderr.write("Inserimento di arco duplicato")
				continue
			hash_map[key] = a
			archi[a.u].append(a)
			rev:Arco = Arco(v, u, w)
			archi[a.v].append(rev)
			nArchi+=1
		elif(campi[0] == "-"):
			u:int = int(campi[1])
			v:int = int(campi[2])
			key = hash((min(u,v),max(u,v))) #Calcola la chiave di hash
			#Se l'arco non esiste avvisa su stderr
			if(key not in hash_map):
				sys.stderr.write("Tentata rimozione di arco non esistente"+ campi[1] + campi[2])
				continue
			#Altrimenti elimina l'arco sia dalla hashmap che dalla lista per msf
			a:Arco = hash_map.pop(key)
			
			archi[a.u].remove(a)
			rev:Arco = Arco(v, u, a.w)
			
			archi[a.v].remove(rev)
			nArchi-=1
#-------------------------------------PRIM---------------------------------------------------
contaVisite:int = 0 #Numero di nodi visitati
visitati:list[bool] = [False] * (nNodi+1) #bitmap dei nodi visitati
msf:list[int] = [] #Nodi nel mst
heap_archi:list[Arco] = [] #Heap degli archi 
sorgente:int

def visita(v): #Dato un nodo, lo segna come visitato e aggiunge gli archi da esso uscenti allo heap
	visitati[v] = True
	for a in archi[v]:
		if(not visitati[a.v]):
			heapq.heappush(heap_archi, a) #Inserisce l'arco nel min heap mantenendo valida la proprietà di heap
		

while(contaVisite < nNodi): #Finchè non sono stativi sitati tutti i nodi
	sorgente = 0 
	#Cerca la sorgente per la visita 
	for i in range (0, nNodi+1):
		if(not visitati[i]): 
			sorgente = i
			break
	#Aumenta il numero di nodi visitati e visita il nodo
	contaVisite+=1 
	visita(sorgente)
	#Per ogni arco nello heap, se il nodo di destinazione non è stato visitato, lo visita e aggiunge l'arco alla MSF 
	while(len(heap_archi) > 0):
		a = heapq.heappop(heap_archi) #Estrae ogni volta l'arco di peso minimo
		if(not visitati[a.v]):
			contaVisite+=1
			visita(a.v)
			hash_map[hash((min(a.u,a.v),max(a.u,a.v)))].msf = True
			costoMSF+=a.w
			numCoCo-=1
print(f"{nArchi} {numCoCo} {costoMSF}")