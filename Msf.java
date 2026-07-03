import java.io.FileReader;
import java.io.BufferedReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Arrays;

public class Msf{
	public static class Arco implements Comparable<Arco>{
		int u, v, w;
		boolean msf;
		Arco(int u, int v, int w){
			this.u = u;
			this.v = v;
			this.w = w;
			this.msf = false;
		}

		@Override
		public int compareTo(Arco other){
			return Integer.compare(this.w, other.w);
		}
	}

	public static int hash(int u, int v){
		long risultato = (u * 2654435761L) ^ (v * 2246822519L);
		return (int) Math.abs(risultato);
	}
	public static int findParent(int[] parent, int component){ //Find con path compression ricorsiva
		if(parent[component] == component) return component;
		return parent[component] = findParent(parent, parent[component]);
	}
	public static void union(int[] parent, int[] rank, int u, int v){ //Union con union by rank 
		int rootU = findParent(parent, u);
		int rootV = findParent(parent, v);
		if(rootU == rootV) return;
		if(rank[rootU] < rank[rootV]){
			parent[rootU] = rootV;
		}else if(rank[rootU] > rank[rootV]){
			parent[rootV] = rootU;
		}else{
			parent[rootV] = rootU;
			rank[rootU]++;
		}
	}

	public static void main(String[] args){
		if(args.length < 2){
			System.err.println("Utilizzo: java Msf file_grafo file_operazioni");
			System.exit(1);
		}
		String file_grafo = args[0];
		String file_operazioni = args[1];
		final HashMap<Integer, Arco> grafo = new HashMap<>(); //Hashmap degli archi
		ArrayList<Arco> archi = new ArrayList<>(); //Arraylist di archi per kruskal
		int nNodi = -1, nArchi = -1, numCoCo;
/*-------------------------------------------LETTURA GRAFO---------------------------------------------------------------- */
		try(BufferedReader in = new BufferedReader(new FileReader(file_grafo))){
			while(true){
				String op = in.readLine();
				if(op == null) break;
				String[] campi = op.trim().split("\\s+");
				

				switch(campi[0]){
					case "c": //Se trova un commento lo ignora 
						continue;
					case "p": //Se trova la linea di configurazione inizializza i campi
						nNodi = Integer.parseInt(campi[2]) + 1 ;
						nArchi = Integer.parseInt(campi[3]);
						break;
					case "a":
						if(nNodi == -1 || nArchi == -1){
							System.err.println("Linea di configurazione non trovata, file non valido");
							System.exit(1);
						}
						//aggiungi arco
						int u = Integer.parseInt(campi[1]);
						int v = Integer.parseInt(campi[2]);
						if(u>v){
							int tmp = u;
							u = v;
							v = tmp;
						}
						int w = Integer.parseInt(campi[3]);
						Arco a  = new Arco(u, v, w);
						grafo.put(Msf.hash(u, v), a);
						archi.add(a);
						break;
					default:
						continue;
				}
			}
		} catch(Exception e){
			System.err.println("Errore: " + e.getMessage());
			e.printStackTrace(System.err);
			System.exit(1);
		}

/*-----------------------------------------------OPERAZIONI--------------------------------------------------- */
	try(BufferedReader in = new BufferedReader(new FileReader(file_operazioni))){
		while(true){
				String op = in.readLine();
				if(op == null) break;
				String[] campi = op.trim().split("\\s+");
				
				switch(campi[0]){
					case "c": //Se trova un commento lo ignora 
						continue;
					case "+": //Se trova la linea di configurazione inizializza i campi
						int u = Integer.parseInt(campi[1]);
						int v = Integer.parseInt(campi[2]);
						if(u>v){
							int tmp = u;
							u = v;
							v = tmp;
						}
						int w = Integer.parseInt(campi[3]);
						Arco a  = new Arco(u, v, w);
						int key = hash(u, v);
						Arco cmp = grafo.get(key);
						if(cmp != null && cmp.equals((a))){
							System.err.println("Arco da inserire già esistente");
							System.exit(1);
						}
						grafo.put(hash(u, v), a);
						archi.add(a);
						nArchi+=1;
						break;
					case "-":
						u = Integer.parseInt(campi[1]);
						v = Integer.parseInt(campi[2]);
						if(u>v){
							int tmp = u;
							u = v;
							v = tmp;
						}
						Arco rem = grafo.remove(hash(u, v));
						if(rem == null){
							System.err.println("Arco da rimuovere non trovato");
							System.exit(1);
						}
						archi.remove(rem);
						nArchi-=1;
						break;
					default:
						continue;
				}
			}
	} catch(Exception e){
		System.err.println("Errore: " + e.getMessage());
		e.printStackTrace(System.err);
		System.exit(1);
	}
/*-------------------------------------------------------------KRUSKAL---------------------------------------------------------------- */
	archi.sort(Arco::compareTo); //Ordina gli archi per peso crescente
	//Array di parent e rank per l'unione dei componenti con path compression
	int[] parent = new int[nNodi]; 
	for(int i = 0; i < nNodi; i++){
		parent[i] = i;
	}
	int [] rank = new int[nNodi];
	Arrays.fill(rank, 1);

	numCoCo = nNodi;
	long costoMSF = 0;
	for(Arco arco : archi){
		int r1 = findParent(parent, arco.u);
		int r2 = findParent(parent, arco.v);
		int w = arco.w;

		if(r1 != r2){
			union(parent, rank, r1, r2);
			arco.msf = true;
			costoMSF += w;
			numCoCo--;
		}
	}
	System.out.println(nArchi + " " + numCoCo + " " + costoMSF);
}
}