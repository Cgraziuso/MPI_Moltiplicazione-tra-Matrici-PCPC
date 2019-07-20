# Moltiplicazione tra Matrici

Il seguente progetto presenta la soluzione alla moltiplicazione di matrici quadrate generate casualmente. In particolare è stato realizzato in modo da poter distribuire il carico del lavoro in base ai processori specificati tramite linea di comando. 

## Esecuzione

Una volta scompattato il pacchetto tar.gz, nella directory ottenuta è possibile trovare un file di nome “MoltiplicazioneMatrici.c”.  Una volta individuato, sarà possibile compilarlo attraverso il seguente comando.

```
mpicc -fopenmp MoltiplicazioneMatrici.c -o progetto
```

A questo punto è possibile eseguire il programma. Il sorgente contiene sia la versione sequenziale che quella parallela: specificando un solo processore tramite linea di comando, sarà possibile eseguire la versione sequenziale. Inoltre per poter eseguire il programma, è necessario specificare, come argomento, la dimensione della matrice in base alla quale verranno create le matrici quadrate.
Ad esempio: 
Se si vuole eseguire il programma con un solo processore e con matrici 20x20 occorre eseguire il seguente comando
```
mpirun -np 1 progetto 20
```

Se si vuole eseguire il programma con due processori e con matrici 20x20 è necessario specificare anche un hostfile al fine di utilizzare eventuali cpu di altre istanze.
```
mpirun -np 2 --hostfile machinefile  progetto 20
```

machinefile sarà costruito nel seguente modo, al posto di privateIpIstance1 verrà inserito l'ip privato dell'istanza slave 1 metre slots indica il numero di vcpu dell'istanza.
```
localhost slots=1
privateIpIstance1 slots=1
privateIpInstance2 slots=1
```


E' inoltre possibile andare ad effettuare moltiplicazioni fra matrici rettangolari. Tale variante può essere eseguita aggiungendo alcuni elementi, come argomenti del comando precedentemente riportato. Fornendo due numeri, il primo rappresenterà il valore comune alle due matici, ovvero le colonne di A e le righe di B, il secondo varierà le rispettive righe di A e colonne di B.
```
mpirun -np 1 --hostfile machinefile  progetto 5 4
```
<img src="https://i.imgur.com/Neglx5f.png" >

Fornendo tre numeri, il primo rappresenterà il valore comune alle due matici, ovvero le colonne di A e le righe di B, il secondo varierà il numero di righe della matrice A ed il terzo numero, varierà il numero di colonne della matrice B. 
```
mpirun --hostfile machinefile  progetto 5 4 3
```
<img src="https://i.imgur.com/4OWfima.png">

per eseguire i test è inoltre necessatio inviare il progetto compilato ad ogni istanza. per far cio è necessario eseguire per ogni nodo il seguente comando:
```
scp progetto pcpc@privateIp:~
```
Dove al posto di privateIp bisogna inserire l'ip privato dell'istanza slave a cui inviare il progetto compilato.

Oltre a *MoltiplicazioneMatrici.c* è stato creato un file *MoltiplicazioneTest.c* che, oltre ad eliminare le stampe delle matrici generate, ripete una stessa esecuzione un numero fissato di volte. Questa scelta è stata fatta al fine di avere una misutazione dei tempi di esecuzione più attendibile. Percio quelli riportati riportati saranno una media dei tempi di esecuzione.
Proprio come avviene per *MoltiplicazioneMatrici.c* questo file di test deve essere compilato e successivamente eseguito. 

```
mpicc -fopenmp MoltiplicazioneTest.c -o progettoTest
..
..// eseguire con matrice quadrata 5x5 
mpirun -np 1 --hostfile machinefile  progettoTest 5
..
..//come prima possono essere eseguite le operazioni sulle matrici rettangolari
mpirun -np 1 --hostfile machinefile  progettoTest 5
mpirun  -np 1 --hostfile machinefile  progettoTest 5 4
mpirun  -np 1 --hostfile machinefile  progettoTest 5 4 3
```

## Prodotto tra matrici. 

Siano A e B due matrici, è possibile eseguire il prodotto fra le due a patto che il numero di colonne della matrice A è uguale al numero di righe della matrice B.
Considerndo le seguenti due matrici:

<img src="https://i.imgur.com/3hiK9Hy.png">

il prodotto tra le due è possibile, in quanto il numero di colonne della matrice A è uguale al numero di righe della matrice B.
A questo punto, considerando la prima riga di A |1 0 2| e le colonne di B |4 3 1| |2 0 2|.
procediamo in questo modo:
|1x4 + 0x3 + 2x1, 1x2 + 0x0 + 2x2|
eseguendo lo stesso calcolo con la seconda riga di A e le colonne di B, la matrice risultante sarà la matrice prodotto
|0x4 + 3x3 + 1x2, 0x2 + 3x0 + 1x2| 

<img src="https://i.imgur.com/Iqy4Wbn.png">

### Sviluppo

Per poter calcolare il prodotto tra due matrici, è stato utilizzato il seguente metodo
```
void resolve_multiplication(int **MatrixA, int **MatrixB, int righeA, int valCom, int colonneB, int **MatrixC)
{
	int i,j,k;
	for (i=0; i<righeA; i++) 
	{
		for (j=0; j<colonneB; j++) 
		{
			MatrixC[i][j] = 0;
			for (k=0; k<valCom; k++) 
			{
			    MatrixC[i][j] = MatrixC[i][j] + MatrixA[i][k]*MatrixB[k][j];
			}
		}
	} 
}

```
come si può osservare, il metodo riceve le matrici A e B, le informazioni relarive alle dimensioni delle matrici e la matrice prodotto C risultante. Nel caso in cui la taglia del problema viene divisa fra più cpu, tale metodo riceverà una matrice parziale di A e ritornerà una porzione della matrice risultante C.

Altro aspetto cardine del problema, è stata la divisione della matrice A tra le diverse CPU. Uno dei vincoli del problema era quello di avere un numero di righe divisibili tra le cpu, nonostante ciò, con tale programma è possibbile utilizzare un numero qualsiasi di righe. Infatti, le righe in eccesso verranno ben distribuite fra i processori e questo non avrà un'impatto significativo sulle prestazioni. 

<img src="https://i.imgur.com/1f6wwQI.png">

La comunicazione della porzione di matrice che ogni processore deve calcolare avviene attraverso la funzione **MPI_ScatterV**: il master invierà ai propri slave la parte della matrice che gli spetta e ogni slave salverà nella matrice *MatrixARcv* ciò che ha ricevuto.
La comunicazione della matrice B avverrà tramite broadcast **MPI_Bcast**.
A questo punto, master e slave potranno calcolare la loro porzione della matrice prodotto e tornare al master la matrice risultante tramite **MPI_GatherV**.
```
    MPI_Scatterv(*matrixA, rowcounts, offset, MPI_INT, *matrixARcv, *rowcounts, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast ( *matrixB, row*row, MPI_INT , 0 , MPI_COMM_WORLD);
    ...
    ...
    MPI_Gatherv( *matrixCRcv , partialRowSize*col , MPI_INT , *matrixC, rowcounts, offset,  MPI_INT , 0, MPI_COMM_WORLD ) ; 
```




Tutte le matrici sono state allocate come dei vettori, in modo da non avere problematiche di memoria non contigua. Di seguito il metodo utilizzato per la generazione delle matrici.
```
int** generate_matrix(int row, int col)
{
	int **M=malloc(row * sizeof(int *));
	M[0] = malloc(row*col*sizeof(int*));
	for(int i = 0; i < row; i++) 
    {
        M[i] = M[0] + i * col;
    }

	return M;
}
```
Di seguito un'immagine che illusta la metodologia di allocazione appena descritta.

<img src="https://i.imgur.com/SOGNgRY.jpg">

## Esecuzione del Benchmark
Questo benchmark è stato eseguito su un cluster di istanze di tipo t2.XLarge di Amazon Web Service usando StarCluster AMI ami-52a0c53b (Linux Ubuntu).
I test sono stati effettuati in termini di Weak Scaling e Strong Scaling utilizzando il file *MoltiplicazioneTest.c* eseguendo dieci iterazioni per ogni istanza. Di seguito sono riportati i risualtati dei test.

### Weak Scalability
 
Per weak scaling si intende la velocità con cui le prestazioni si degradano all'aumentare dei core ma con dimensione del problema invariata per ogni processore. 
In pratica, all'aumentare dei core, la nuova taglia del problema TN sara di TN = T1 * numero di processori (T1 è il tempo necessario con un processore).
Tali test, quindi, prevedono che ogni processore abbia un numero costante di operazioni. 
Nel problema del prodotto tra matrici, l'utilizzo delle matrici quadrate non permette un test esaustivo per il weak scaling.
Infatti, considerando due matrici quadrate di 500 righe e colonne, raddoppiando la taglia del problema, ovvero due matrici da 1000 righe e colonne, 
il peso computazionale da distribuire non risulta il medesimo della precedente iterazione.
Infatti raddoppiando entrambi le matrici A e B la taglia del problema aumenta più di quattro volte. 
Per effettuare un test adeguato, si pensato di utilizzare le matrici rettangolari, infatti, raddoppiando soltanto le righe della
matrice A e lasciando invariate i restanti valori, è possibile assegnare ad ogni processore la stessa taglia del problema. 
Di seguito un'immagine illustrativa del principio appena destritto.

<img src="https://i.imgur.com/avbP46m.png">

Per i test si è scelti di partire da un prodotto di due matrici quadrate di 500 righe e colonne.
Per ogni vcpu in più, si aggiungono 500 righe solo alla matrice A. 
Al fine di comprendere meglio quanto è l'efficienza dell'algoritmo, si utilizzerà la **Weak Scaling Efficency** che risulta essere il
tempo necessario per completare un'unità di lavoro con 1 cpu (p1), diviso la quantità di tempo per completare con N
unità di lavoro utilizzando N cpu (pN). L'efficienza è data da: (p1 / pN) * 100%.

Di seguito una tablebella che illusta l'esperimento.

| Grandezza matrice A  | N. Core | Tempo (s)   | Weak Scaling Efficency | 
| :---------:          | :-----: | :---------: | :-----: |
| 500x500              | 1       | 0,923s      | x       |
| 1000x500             | 2       | 0,956s      | 96,54%  |
| 1500x500             | 3       | 0,946s      | 97,56%  |
| 2000x500             | 4       | 0,956s      | 96,54%  |
| 2500x500             | 5       | 0,974s      | 94,76%  |
| 3000x500             | 6       | 0,995s      | 92,76%  |
| 3500x500             | 7       | 1,02s       | 90,49%  |
| 4000x500             | 8       | 1,039s      | 88,83%  |
| 4500x500             | 9       | 1,046s      | 88,24%  |
| 5000x500             | 10      | 1,068s      | 86,42%  |
| 5500x500             | 11      | 1,077s      | 85,70%  |
| 6000x500             | 12      | 1,099s      | 83,98%  |
| 6500x500             | 13      | 1,109s      | 83,22%  |
| 7000x500             | 14      | 1,118s      | 82,55%  |
| 7500x500             | 15      | 1,133s      | 81,46%  |
| 8000x500             | 16      | 1,149s      | 80,33%  |
| 8500x500             | 17      | 1,179s      | 78,28%  |
| 9000x500             | 18      | 1,181s      | 78,15%  |
| 9500x500             | 19      | 1,191s      | 77,49%  |
| 10000x500            | 20      | 1,218s      | 75,77%  |



<img src="https://i.imgur.com/hufpRdQ.jpg">

Come si può notare dal grafico, all'aumentare dei core e linermente la taglia del problema, le tempistiche risultano non variare molto. 
Abbiamo una variazione rispetto al programama lineare di massimo 75%. Aspetto cardine da considerare sono le tempistiche di divisione della matrice A e
conseguente ricezione della matrice prodotto, oltre che alle tempistiche dovute all'overhead di comunicazione all'aumentare dei core. 

### Strong Scalability

Nella fase di strong scaling è stato testato il programma parallelo utilizzando una matrice con taglia 2000x2000 ,
variando solamente il numero di istanze nel cluster e conseguentemente di processori. In questo caso si va ad analizzare il comportamento dell'algoritmo all'aumentare dei processori utilizzati.
Per comprendere meglio il trade-off tra i processori utilizzati e le prestazioni ottenute introduciamo 
il concetto di **Strong Scaling Efficiency**: il tempo necessario per completare un'unità di lavoro con 1 cpu (p1)diviso 
la quantità di tempo per completare la stessa unità di lavoro con N cpu (pN) moltiplicato per N, la **Strong Scaling Efficiency** (in percentuale di lineare)
è data da: p1 / (N * pN) * 100%

| Grandezza delle matrici | N. Core | Tempo (s)   | Strong Scaling Efficiency | 
| :----------------------:| :-----: | :---------: | :-----: |
| 2000x2000               | 1       | 84,658s     | x       |
| 2000x2000               | 2       | 42,966s     | 98,51%  |
| 2000x2000               | 3       | 29,906s     | 94,35%  |
| 2000x2000               | 4       | 23,340s     | 90,67%  |
| 2000x2000               | 5       | 19,297s     | 87,73%  |
| 2000x2000               | 6       | 16,503s     | 85,49%  |
| 2000x2000               | 7       | 14,471s     | 83,57%  |
| 2000x2000               | 8       | 12,949s     | 81,71%  |
| 2000x2000               | 9       | 11,599s     | 81,09%  |
| 2000x2000               | 10      | 10,573s     | 80,06%  |
| 2000x2000               | 11      | 9,0650s     | 79,75%  |
| 2000x2000               | 12      | 8,842s      | 79,78%  |
| 2000x2000               | 13      | 8,242s      | 79,01%  |
| 2000x2000               | 14      | 7,662s      | 78,92%  |
| 2000x2000               | 15      | 6,536s      | 86,34%  |
| 2000x2000               | 16      | 6,864s      | 77,97%  |
| 2000x2000               | 17      | 6,306s      | 78,97%  |
| 2000x2000               | 18      | 5,971s      | 78,76%  |
| 2000x2000               | 19      | 5,814s      | 76,63%  |
| 2000x2000               | 20      | 5,463s      | 77,47%  |

<img src="https://i.imgur.com/TaajnW7.jpg">


Com'è possibile notare, utilizzando 20 processori e due matrici 2000X2000 abbiamo un'indice di efficienza pari al 77%. È importante sottolineare che le prestazioni calano linearmente a causa del sovraccarico della comunicazione. Questo significa che per test in cui l'input è simile a quello utilizzato, è inutile adottare più di 10 processori in quanto al di sotto di questa soglia abbiamo prestazioni minori all'80%. Nonostante ciò, con un input di dimensioni maggiori, più di 10 cpu contribuirebbero ad aumentare le prestazioni, ma anche in questo caso ci sarà un punto in cui queste non miglioreranno troppo rispetto al numero di processori.
In generale abbiamo un **Strong Scaling Efficiency** massimo dell'98% con due processori ed uno minimo del 76% con 19 processori, con l'input dato e 20 cpu abbiamo un miglioramnto del 77% che, anche se non ottimale, risulta essere un buon risultato. Riproporre un test su istanze ben più grandi mostrerebbe meglio il comportamento dell'algoritmo all'aumentare dei core e lo scaling che raggiungerebbe con 20 cpu.


### Candidato

Il seguente progetto è stato realizzato da Graziuso Catello 0522500680 per l'esame di Programmazione concorrente, parallela su Coud



















