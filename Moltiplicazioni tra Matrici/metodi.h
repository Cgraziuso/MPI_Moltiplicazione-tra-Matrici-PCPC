//Graziuso Catello 0522500680

//metodo utile alla risoluzione della moltiplicazione di matrici. prende in input le due matrici da moltiplicare, i valori relativi alla dimensioni delle matrici
//e la matrice C risultante.
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

//questo metodo alloca una matrice attraverso la malloc. al fine di non avere problemi di memoria contigua, ho trattato la matrice come un vettore monodimentsionale
//andando poi a specificare i puntatori alle colonne
int** alloc_matrix(int row, int col)
{
	int **M=malloc(row * sizeof(int *));
	M[0] = malloc(row*col*sizeof(int*));
	for(int i = 0; i < row; i++) 
    {
        M[i] = M[0] + i * col;
    }

	return M;
}

//questo metodo è utile a calcolare il numero di righe che ogni processo dovrebbe ricevere della matriceA. In base al proprio rank, il processore calcola il numero di righe verificando anche il resto.
int calculate_size(int worldSize, int row, int rank)
{
	int quoziente = row/worldSize;
	int resto = row % worldSize;
	if(rank<resto)return quoziente+1;
	else return quoziente;
}

//questo metodo riempie la matriche di numeri casuali da 1 a 100
int** random_matrix(int **matrix, int row, int col)
{
	for(int i = 0; i < row; i++) 
        {
            for(int j = 0; j < col; j++) {
               *(*(matrix +i) +j) = (rand() %100) +1;
                printf("%d\t", *(*(matrix +i) +j));
            }
            printf("\n");
        }

	return matrix;
}

//questo metodo è utile a calcolare i vettori relativi alla scatterV e gatherV. Infatti, al fine di una perfetta divisione delle righe, occorre conoscere
//quante righe deve ricevere ogni processore (rowcounts) e lo spostamento che bisogna effettuare nell'invio della matrice (offset).
void calculate_ScatterV_elements(int *rowcounts, int *offset, int resto, int sum, int size, int row, int colrow)
{
	for (int i = 0; i < size; i++) 
	 {
        rowcounts[i] = row/size;
        if (resto> 0) {
            rowcounts[i]++;
     		offset[i] = sum;
     		rowcounts[i]= rowcounts[i]*colrow;
     		sum = sum + rowcounts[i];
            resto--;
        }else{
        	offset[i] = sum;
        	rowcounts[i]= rowcounts[i]*colrow;
     		sum = sum + rowcounts[i];
        }
   
      }
}
