/*Código apenas introdutório (Commit 2 - Implementando mais atribuições pós testes)*/

#include <stdio.h> // Biblioteca utilizada para o printf()
#include <stdlib.h> // Alocação de memória e conversão de tipo de dados
#include <pthread.h> // Biblioteca para criar e manipular threads
#include <unistd.h> // Algumas funções como o usleep que será utilizado no desenvolvimento

#define NUM_THREADS 3
#define TOTAL_PER_UPDATE 20
#define INTERVALO_DISPLAY 2

double peso_total = 0;
int total_produtos = 0;
double peso_esteira[3] = {0,0,0};

pthread_mutex_t semaforo = PTHREAD_MUTEX_INITIALIZER;

main(){
    
    return 0;
}