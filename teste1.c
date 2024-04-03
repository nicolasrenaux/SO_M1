#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_THREADS 3
#define INTERVALO_DISPLAY 2
#define LIMITE_PRODUTOS 100

double peso_esteira[NUM_THREADS] = {0, 0, 0};
int count[NUM_THREADS] = {0, 0, 0};

pthread_mutex_t mutex_contagem = PTHREAD_MUTEX_INITIALIZER;

// Pipes
int pipe_display[2];

void* esteira(void* arg) {
    long id = (long)arg;
    double peso;
    double intervalo;

    if (id == 0) {
        peso = 5.0;
        intervalo = 1.0;
    } else if (id == 1) {
        peso = 2.0;
        intervalo = 0.5;
    } else {
        peso = 0.5;
        intervalo = 0.1;
    }

    while (1) {
        usleep(intervalo * 1000000);

        pthread_mutex_lock(&mutex_contagem);
        peso_esteira[id] += peso;
        count[id]++;
        pthread_mutex_unlock(&mutex_contagem);
    }

    return NULL;
}

// Função do processo filho para exibir os resultados
void exibe_resultados() {
    double peso[NUM_THREADS];
    int local_count[NUM_THREADS];

    while (1) {
        read(pipe_display[0], &peso, sizeof(peso)); // Lê pesos atualizados
        read(pipe_display[0], &local_count, sizeof(local_count)); // Lê contagens atualizadas

        printf("Atualização dos valores:\n");
        for (int i = 0; i < NUM_THREADS; i++) {
            printf("Esteira %d: Produtos = %d, Peso = %.2f kg\n", i + 1, local_count[i], peso[i]);
        }
        sleep(INTERVALO_DISPLAY);
    }
}

int main() {
    pthread_t threads[NUM_THREADS];

    // Cria o pipe para comunicação
    if (pipe(pipe_display) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) { // Processo filho para exibição de resultados
        close(pipe_display[1]); // Fecha o lado de escrita no filho
        exibe_resultados();
        exit(0);
    } else if (pid > 0) { // Processo pai
        close(pipe_display[0]); // Fecha o lado de leitura no pai

        for (long i = 0; i < NUM_THREADS; i++) {
            pthread_create(&threads[i], NULL, esteira, (void*)i);
        }

        // Atualiza os dados a serem exibidos a cada INTERVALO_DISPLAY segundos
        while (1) {
            sleep(INTERVALO_DISPLAY);
            pthread_mutex_lock(&mutex_contagem);
            write(pipe_display[1], &peso_esteira, sizeof(peso_esteira));
            write(pipe_display[1], &count, sizeof(count));
            pthread_mutex_unlock(&mutex_contagem);
        }

        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }

        wait(NULL); // Espera o processo filho terminar
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}
