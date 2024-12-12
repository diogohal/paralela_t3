CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lpthread

# Nome do executável
all: main

# Regra para compilar o executável
main: main.o verifica_particoes.o
	$(CC) $(CFLAGS) -o main main.o verifica_particoes.o $(LDFLAGS)

# Regra para compilar main.o
main.o: main.c verifica_particoes.h
	$(CC) $(CFLAGS) -c main.c

# Regra para compilar verifica_particoes.o
verifica_particoes.o: verifica_particoes.c verifica_particoes.h
	$(CC) $(CFLAGS) -c verifica_particoes.c

clean:
	rm *.o main