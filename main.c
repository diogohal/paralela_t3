#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <mpi.h>
#include "verifica_particoes.h"
#define RAND_MAX_CUSTOM 10

typedef struct {
    int size;
    int maxSize;
    long long *array;
} localOutput_t;

void insereLocalOutput(localOutput_t *localOutput, long long num) {
    if (localOutput->size == localOutput->maxSize) {
        long long *temp = realloc(localOutput->array, sizeof(long long) * localOutput->maxSize * 2);
        if (!temp) {
            perror("Realloc failed");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        localOutput->array = temp;
        localOutput->maxSize *= 2;
    }
    localOutput->array[localOutput->size++] = num;
}

int procuraVetor(long long *P, int tamanho, long long x) {
    int inicio = 0, fim = tamanho - 1, meio;

    while (inicio <= fim) {
        meio = inicio + (fim - inicio) / 2;

        if (P[meio] == x) {
            return meio;
        } else if (P[meio] < x) {
            inicio = meio + 1;
        } else {
            fim = meio - 1;
        }
    }

    return inicio;
}

long long geraAleatorioLL() {
    return (long long)rand() * 100 + rand();
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
    if (!vetor) return NULL;

    for (int i = 0; i < n; i++) {
        vetor[i] = geraAleatorioLL();
    }

    if (ordena) {
        qsort(vetor, n, sizeof(long long), compara);
        vetor[n - 1] = LLONG_MAX;
    }

    return vetor;
}

int *geraVetorPos(int n) {
    int *vetor = calloc(n, sizeof(int));
    if (!vetor) return NULL;
    return vetor;
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        if (rank == 0) {
            printf("Uso: %s <n> <np>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int n = atoi(argv[1]);
    int np = atoi(argv[2]);

    long long *Input = NULL;
    long long *P = NULL;
    long long *Output = malloc(sizeof(long long) * n);
    int *Pos = NULL;

    if (rank == 0) {
        Input = geraVetor(n, 0);
        P = geraVetor(np, 1);
        Pos = geraVetorPos(np);
    }

    long long *localInput = malloc(sizeof(long long) * (n / size));
    int localSize = n / size;

    MPI_Bcast(P, np, MPI_LONG_LONG, 0, MPI_COMM_WORLD);
    MPI_Scatter(Input, localSize, MPI_LONG_LONG, localInput, localSize, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    localOutput_t localOutput = {0, localSize, malloc(sizeof(long long) * localSize)};
    int *localPos = calloc(np, sizeof(int));

    for (int i = 0; i < localSize; i++) {
        int pos = procuraVetor(P, np, localInput[i]);
        localPos[pos]++;
        insereLocalOutput(&localOutput, localInput[i]);
    }

    MPI_Reduce(localPos, Pos, np, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        somaAcumulativa(Pos, np);
        verifica_particoes(Input, n, P, np, Output, Pos);
    }

    free(localInput);
    free(localOutput.array);
    free(localPos);
    if (rank == 0) {
        free(Input);
        free(P);
        free(Pos);
        free(Output);
    }

    MPI_Finalize();
    return 0;
}
