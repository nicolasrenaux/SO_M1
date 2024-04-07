#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_ESTEIRAS 3
#define UNIDADES_PESAGEM 100

int contagens[NUM_ESTEIRAS];
double pesos[UNIDADES_PESAGEM]; // Vetor para armazenar pesos
pthread_mutex_t mutex_contagem;

void *contagem_esteira(void *arg) {
    int id_esteira = *(int *)arg;
    while (1) {
        // Simular tempo de passagem de item
        switch (id_esteira) {
            case 0: sleep(1); break; // Esteira 1
            case 1: usleep(500000); break; // Esteira 2
            case 2: usleep(100000); break; // Esteira 3
        }
        pthread_mutex_lock(&mutex_contagem);
        contagens[id_esteira]++;
        pthread_mutex_unlock(&mutex_contagem);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_ESTEIRAS];
    int i, total_contagem;
    
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("Erro ao criar pipe");
        exit(1);
    }

    // Criar threads para cada esteira
    for (i = 0; i < NUM_ESTEIRAS; i++) {
        if (pthread_create(&threads[i], NULL, contagem_esteira, &i) != 0) {
            perror("Erro ao criar thread");
            exit(1);
        }
    }

    // Loop principal para enviar contagem e peso total pelo pipe
    while (1) {
        sleep(2); // Enviar a cada 2 segundos
        total_contagem = 0;
        for (i = 0; i < NUM_ESTEIRAS; i++) {
            total_contagem += contagens[i];
        }
        write(pipe_fd[1], &total_contagem, sizeof(int));

        // (implementar lógica para calcular e enviar peso total)
    }

    // Esperar threads terminarem (não alcançado neste exemplo)
    for (i = 0; i < NUM_ESTEIRAS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_mutex_destroy(&mutex_contagem);
    return 0;
}