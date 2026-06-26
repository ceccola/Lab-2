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

nNodi:int = 0
nArchi:int = 0
costoMSF:int = 0
numCoCo:int 
hash_map:dict = {}

if(len(sys.argv) < 3):
	print("Utilizzo: msf.py file_grafo file_archi")
	sys.exit(1)

#File per grafo e archi
file_grafo:str = sys.argv[1]
file_operazioni:str = sys.argv[2] 
archi:list[list[Arco]] = [] #Lista di archi per ricerca msf

#-------------------------------------LETTURA FILE GRAFO--------------------------------------------------
with open(file_grafo, "r") as file:
	for linea in file: 
		campi = linea.strip().split()
		if(campi[0] == "c"): #Ignora i commenti e continua
			continue
		elif(campi[0] == "p"): #Configura il numero di nodi e di archi
			nNodi = int(campi[2]) +1
			archi = [[] for _ in range(nNodi +1)]
			nArchi = int(campi[3])
		elif(campi[0] == "a"): #Inizializza l'arco e i suoi campi e inserisce nella hashtable
			u:int = int(campi[1])
			v:int = int(campi[2])
			w:int = int(campi[3])
			a:Arco = Arco(u, v, w)
			key:int = hash((min(u,v),max(u,v)))
			hash_map[key] = a
			archi[a.u].append(a) #Aggiunge l'arco alla lista degli archi
			rev:Arco = Arco(v, u, w)
			archi[a.v].append(rev)
numCoCo = nNodi 

#-------------------------------------LETTURA OPERAZIONI---------------------------------------------------
with open(file_operazioni) as file:
	for linea in file:
		campi = linea.strip().split()
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
contaVisite:int = 0
visitati:list[bool] = [False] * (nNodi+1)
msf:list[int] = [] #Nodi nel mst
heap_archi:list[Arco] = []
sorgente:int

def visita(v):
	visitati[v] = True
	for a in archi[v]:
		if(not visitati[a.v]):
			heapq.heappush(heap_archi, a)
		

while(contaVisite < nNodi):
	sorgente = 0
	for i in range (0, nNodi+1):
		if(not visitati[i]): 
			sorgente = i
			break
	contaVisite+=1
	visita(sorgente)
	while(len(heap_archi) > 0):
		a = heapq.heappop(heap_archi)
		if(not visitati[a.v]):
			contaVisite+=1
			visita(a.v)
			hash_map[hash((min(a.u,a.v),max(a.u,a.v)))].msf = True
			costoMSF+=a.w
			numCoCo-=1
print(f"{nArchi} {numCoCo} {costoMSF}")