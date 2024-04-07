#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>

#define NUM_THREADS 3
#define INTERVALO_DISPLAY 2
#define LIMITE_PRODUTOS 100

double peso_esteira[NUM_THREADS] = {0, 0, 0};
int count[NUM_THREADS] = {0, 0, 0};
int total_produtos = 0;

pthread_mutex_t mutex_contagem = PTHREAD_MUTEX_INITIALIZER;

int pipe_display[2];
int sistema_pausado = 0;
int pausa_toggle = 0;

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
        if (!sistema_pausado) {
            peso_esteira[id] += peso;
            count[id]++;
            if (count[id] % LIMITE_PRODUTOS == 0) {
                total_produtos += LIMITE_PRODUTOS;
                printf("\nLimite de %d produtos atingido. Total de peso das esteiras: %.2f kg\n", total_produtos, peso_esteira[0] + peso_esteira[1] + peso_esteira[2]);
                sleep(2);
            }
        }
        pthread_mutex_unlock(&mutex_contagem);
    }

    return NULL;
}

void* controlador(void* arg) {
    printf("Pressione ENTER para pausar ou continuar o sistema.\n");

    while (1) {
        getchar();

        pthread_mutex_lock(&mutex_contagem);
        if (!pausa_toggle) {
            printf("Sistema pausado.\n");
            sistema_pausado = 1;
        } else {
            printf("Sistema continuando.\n");
            sistema_pausado = 0;
        }
        pausa_toggle = !pausa_toggle;
        pthread_mutex_unlock(&mutex_contagem);
    }

    return NULL;
}

void exibe_resultados() {
    double peso[NUM_THREADS];
    int local_count[NUM_THREADS];

    while (1) {
        pthread_mutex_lock(&mutex_contagem);
        read(pipe_display[0], &peso, sizeof(peso));
        read(pipe_display[0], &local_count, sizeof(local_count));
        pthread_mutex_unlock(&mutex_contagem);

        if (!sistema_pausado) {
            printf("\nAtualização dos valores:\n");
            for (int i = 0; i < NUM_THREADS; i++) {
                printf("Esteira %d: Produtos = %d, Peso = %.2f kg\n", i + 1, local_count[i], peso[i]);
            }
        }

        sleep(INTERVALO_DISPLAY);
    }
}


int main() {
    pthread_t threads[NUM_THREADS];
    pthread_t thread_control;

    if (pipe(pipe_display) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) {
        close(pipe_display[1]);
        exibe_resultados();
        exit(0);
    } else if (pid > 0) {
        close(pipe_display[0]);

        for (long i = 0; i < NUM_THREADS; i++) {
            pthread_create(&threads[i], NULL, esteira, (void*)i);
        }
        pthread_create(&thread_control, NULL, controlador, NULL);

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
        pthread_join(thread_control, NULL);

        wait(NULL);
    } else {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}
