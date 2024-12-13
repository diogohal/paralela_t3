// Diogo Almeida GRR20210577
// Bruno Spring GRR20211279

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "mpi.h"
#include <limits.h>
#include "chrono.h"
#include "verifica_particoes.h"
#define NTIMES 10

long long geraAleatorioLL() {
    int a = rand();
    int b = rand();
    long long v = (long long)a * 100 + b;
    return v;
}  

int compara(const void *a, const void *b) {
    long long valA = *(const long long *)a;
    long long valB = *(const long long *)b;
    if (valA < valB) return -1;
    if (valA > valB) return 1;
    return 0;
}

long long *geraVetor(int n, int ordena) {
    long long *vetor = malloc(sizeof(long long) * n);
    if (!vetor)
        return NULL;
    
    for (long long i = 0; i < n; i++) {
        vetor[i] = geraAleatorioLL();
    }
    
    if (ordena) {
        qsort(vetor, n, sizeof(long long), compara);
        vetor[n-1] = LLONG_MAX;
    }
    
    return vetor;
}

void printVetor(long long *Output, int n) {
    for(int i=0; i<n; i++) {
        printf("%lld ", Output[i]);
    }
    printf("\n");
}

void printVetorPos(int *Pos, int n) {
    for(int i=0; i<n; i++) {
        printf("%d ", Pos[i]);
    }
    printf("\n");
}


long long procuraVetor(long long arr[], int tamanho, long long x) {
    long long inicio = 0, fim = tamanho - 1;
    long long retorno = tamanho;

    while (inicio <= fim) {
        long long meio = inicio + (fim - inicio) / 2;

        if (arr[meio] > x) {
            retorno = meio;
            fim = meio - 1;
        }
        else {
            inicio = meio + 1;
        }
    }
    return retorno;
}

void multi_partition_mpi(long long* Input, int n, long long* P, int np, long long* Output, int* Pos) {
    int processId;
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);

    long long* localOutput = malloc(sizeof(long long) * np * n);

    int* sendCounts = malloc(sizeof(int) * np);
    for (int i = 0; i < np; i++) {
        sendCounts[i] = 0;
    }

    for (int i = 0; i < n; i++) {
        int retorno = procuraVetor(P, np, Input[i]);
        localOutput[retorno * n + sendCounts[retorno]] = Input[i];
        sendCounts[retorno]++;
    }

    int* recvCounts = malloc(sizeof(int) * np);
    MPI_Alltoall(sendCounts, 1, MPI_INT,
                recvCounts, 1, MPI_INT, MPI_COMM_WORLD);

    int* sendDispls = malloc(sizeof(int) * np);
    sendDispls[0] = 0;

    int* recvDispls = malloc(sizeof(int) * np);
    recvDispls[0] = 0;

    for (int i = 1; i < np; i++) {
        sendDispls[i] = i * n;
        recvDispls[i] = recvDispls[i - 1] + recvCounts[i - 1];
    }

    *Pos = recvDispls[np-1] + recvCounts[np-1];

    MPI_Alltoallv(localOutput, sendCounts, sendDispls, MPI_LONG_LONG,
                Output, recvCounts, recvDispls, MPI_LONG_LONG, MPI_COMM_WORLD);


    free(localOutput);
    free(sendCounts);
    free(recvCounts);
    free(sendDispls);
    free(recvDispls);

    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("mpirun -np <numeroProcessos> ./main <tamanhoVetor>\n");
        return 1;
    }

    int processId, np, n, Pos;
    long long *Input, *P, *Output;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &processId);
    srand(2024 * 100 + processId);

    int nTotalElements = atoi(argv[1]);
    n = nTotalElements / np;

    for (int i = 0; i < NTIMES; i++){
        Input = geraVetor(nTotalElements, 0);
        Output = geraVetor(nTotalElements, 0);
        P = geraVetor(np, 1);

        MPI_Bcast(P, np, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
        chronometer_t timer;
        chrono_reset(&timer);
        chrono_start(&timer);
        
        multi_partition_mpi(Input, n, P, np, Output, &Pos);
        // verifica_particoes(Input, n, P, np, Output, &Pos);
        chrono_stop(&timer);

        if (processId == 0) {
            double tempo_medio = (double) chrono_gettotal(&timer) / (1000 * 1000 * 1000);
            double vazao = nTotalElements / tempo_medio/1000000;                
            printf("Tempo médio: %lf s\n", tempo_medio);
            printf("Vazão: %lf MEPS/s\n", vazao);
        }

        free(Input);
        free(P);
        free(Output);
    }

    MPI_Finalize();

    return 0;
}