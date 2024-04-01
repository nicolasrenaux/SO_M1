#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 3
#define INTERVALO_DISPLAY 2
#define LIMITE_PRODUTOS 100

double peso_total = 0;
int total_produtos = 0;
double peso_esteira[NUM_THREADS] = {0, 0, 0};
int count[NUM_THREADS] ={0,0,0};

pthread_mutex_t semaforo = PTHREAD_MUTEX_INITIALIZER;

// Função para cada esteira
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
        usleep(intervalo * 1000000); // Convertendo segundos para microsegundos

        pthread_mutex_lock(&semaforo);
        peso_esteira[id] += peso;
        count[id]++;


        total_produtos++;

        // peso_total += peso;
        pthread_mutex_unlock(&semaforo);
    }

    return NULL;
}

// Função para a thread que exibe resultados
void* exibe_resultados(void* arg) {
    while (1) {
        usleep(INTERVALO_DISPLAY * 1000000); // Convertendo segundos para microsegundos

        pthread_mutex_lock(&semaforo);
        printf("Total de produtos: %d\n", total_produtos);
        for (int i = 0; i < NUM_THREADS; i++) {
            printf("Esteira %d: Quantidade Produtos: %d Peso da esteira: %.2f kg\n", i + 1, count[i], peso_esteira[i]);
        }

        if (total_produtos >= LIMITE_PRODUTOS) {
            for(int i = 0; i<NUM_THREADS; i++){
                peso_total+=peso_esteira[i];
            }

            printf("Atenção: Total de produtos atingiu %d.\n", LIMITE_PRODUTOS);
            printf("Peso total: %.2f kg\n\n", peso_total);
            // Podemos terminar o programa ou continuar. Exemplo para terminar:
            total_produtos = 0;
            usleep(5.0 * 1000000); // Espera 5 segundos
        }
        pthread_mutex_unlock(&semaforo);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    pthread_t thread_display;

    // Criando threads para as esteiras
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, esteira, (void*)i);
    }

    // Criando thread para exibição
    pthread_create(&thread_display, NULL, exibe_resultados, NULL);

    // Aguardando as threads terminarem (o que não acontecerá neste exemplo)
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    pthread_join(thread_display, NULL);

    return 0;
}
