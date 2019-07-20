#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include "metodi.h"

//Graziuso Catello 0522500680

int main(int argc, char *argv[]) 
{
	int rank, size, partialRowSize; 	// rank dei processi, size dell'MPI_Comm, righe parziali che ogni processore deve analizzare
	int row, colrow, col; 				// righe matrice A, colonne della matrice A e righe della matrice B, colonne matrice B
    int **matrixA; 						
    int **matrixB; 						
    int **matrixC; 
    float timeStart, finalTime;  							

    int **matrixARcv;					//matrice parziale della matrice A. Verra utilizzata nella ScatterV per ricevere una porzione della matrice A
    int **matrixCRcv;					//matrice parziale della matrice C. Verra utilizzata per calcolare la porzione della matrice finale

    int *rowcounts;    					// array  che descrive quanti elementi mandare ad ogni processo
    int *offset;       					// array che descrive gli offset in cui inizia ogni porzione inviata nella ScatterV
    int resto;		   					// variabile utile al calcolo delle porzioni per la ScatterV
    int sum=0;		   					// variabile utile al calcolo di offset

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status status;

	srand(time(NULL)); 

	if(argc == 2) //in caso di un solo argomento da linea di comando, le matrici generate saranno quadrate secondo il valore di argc
	{
		row = atoi(argv[1]);  		
		colrow = atoi(argv[1]); 	
		col = atoi(argv[1]);  		

		partialRowSize= calculate_size(size, row, rank); 	

	}else if(argc == 3){ //in caso di due argomenti, il primo sarà il valore comune alla matriceA e matriceB (requisito necessario per la moltiblicazione), il secondo rispettivamente le righe della Matrice A e le colonne della matrice B

		colrow = atoi(argv[1]); 
		row = atoi(argv[2]); 	
		col = atoi(argv[2]); 	
		
		partialRowSize= calculate_size(size, row, rank); 

	}else if(argc == 4){ //in caso di tre argomenti, variano i tre valori della matrice. 

		colrow = atoi(argv[1]); 
		row= atoi(argv[2]); 	
		col = atoi(argv[3]);	

		partialRowSize= calculate_size(size, row, rank); 

	}

	//Generazioni delle matrici A,B e C dinamice 
	matrixA=alloc_matrix(row, colrow); 
	matrixB=alloc_matrix(colrow, col); 
	matrixC=alloc_matrix(row, col);	 


	//il master inizializza le matrici e risolve il problema se risulta essere l'unico processore
	if(rank == 0) 
	{
		printf("Matrix A\n"); 
        random_matrix(matrixA, row, colrow);

        printf("Matrix B\n"); 
        random_matrix(matrixB, colrow, col);

        //codice sequenziale
        if(size==1) 
        {
	        timeStart = MPI_Wtime(); 											//inizo tempo MPI
	        resolve_multiplication(matrixA, matrixB, row, colrow, col, matrixC);
	        finalTime = MPI_Wtime() - timeStart; 								//fine tempo MPI

			printf("MatriXC\n");
			for(int i=0; i<row; i++)
			{
				for(int j=0; j<col; j++)
				{
					printf("%d ", matrixC[i][j]);
				}
				printf("\n");
			}
			
			printf("Tempo sequenziale= %f\n", finalTime );
			 
			free(matrixA); 
			free(matrixB);
			free(matrixC);
			MPI_Finalize();
    		return 0;
        }
    }//fine master inizio parallelo

    MPI_Barrier(MPI_COMM_WORLD);
    if(rank==0) timeStart = MPI_Wtime();										   //inizo tempo MPI
	rowcounts = malloc(sizeof(int)*size); 										   //allocazione dinamica di rowcounts ed offset
	offset = malloc(sizeof(int)*size);	   										   
	resto = row % size;				   											  
	calculate_ScatterV_elements(rowcounts, offset, resto, sum, size, row, colrow); 

	matrixARcv = alloc_matrix(partialRowSize ,colrow);							   //allocazione dinamic matriceARcv e matrixCRcv 
    matrixCRcv = alloc_matrix(partialRowSize, col);			
    if(matrixB == NULL) matrixB = alloc_matrix(colrow, col);					   //in caso di processi con rank diverso da 0, allocano la matrice B	 
        

    MPI_Barrier(MPI_COMM_WORLD); 																				//mi assicuro che tutti i processi arrivino a questo punto
    MPI_Scatterv(*matrixA, rowcounts, offset, MPI_INT, *matrixARcv, *rowcounts, MPI_INT, 0, MPI_COMM_WORLD);	//divido la matriceA in base alla porzione che ogni processo deve ricevere
    MPI_Bcast ( *matrixB, colrow*col, MPI_INT , 0 , MPI_COMM_WORLD);												//eseguo un broadcast della matrice B

    sum = 0;																				//inizializzo di nuovo sum a 0 e resto a row % size  al fine di calcolare le porzioni della matrice C che il master deve ricevere
    resto = row % size;																		
    calculate_ScatterV_elements(rowcounts, offset, resto, sum, size, row, col);				
	resolve_multiplication(matrixARcv, matrixB, partialRowSize, colrow, col, matrixCRcv);	

	
	MPI_Gatherv( *matrixCRcv , partialRowSize*col , MPI_INT , *matrixC, rowcounts, offset,  MPI_INT , 0, MPI_COMM_WORLD ) ;  //ricezione della matriceC
	


	if(rank==0)//soltanto il master stampa la matrice risultante
	{
		finalTime = MPI_Wtime() - timeStart; //fine tempo MPI
		printf("MatrixC\n");
		for(int i=0; i<row; i++)
		{
			for(int j=0; j<col; j++)
			{	
				printf("%d\t",matrixC[i][j]);
			}
		printf("\n");
		}
		
		printf("Tempo parallelo= %f\n", finalTime );
	}
	free(matrixA);
	free(matrixB);
	free(matrixC);
	free(matrixARcv);
	free(matrixCRcv);
	free(rowcounts);
	free(offset);

	MPI_Finalize();
    return 0;


}
