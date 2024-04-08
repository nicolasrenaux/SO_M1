#include <stdio.h>  //Biblioteca padrão de entrada e saída.
#include <stdlib.h>  //Necessária para funções como exit().
#include <unistd.h>  // Biblioteca utilizada para write(), read() e usleep()
#include <pthread.h>  // Biblioteca para usar as threads
#include <sys/wait.h>  // Necessária para utilizar o wait()

#define NUM_THREADS 3 // Define o número de threads para esteiras
#define INTERVALO_DISPLAY 2 // Intervalo de tempo para exibir resultados
#define LIMITE_PRODUTOS 1500 // Limite de produtos antes de pausar para exibir resultados

double peso_esteira[NUM_THREADS] = {0, 0, 0}; // Armazena o peso total em cada esteira
int count[NUM_THREADS] = {0, 0, 0}; // Conta o número de produtos em cada esteira
int total_produtos = 0; // Conta o total de produtos processados por todas as esteiras

pthread_mutex_t mutex_contagem = PTHREAD_MUTEX_INITIALIZER; // Mutex para controlar acesso à seção crítica

int pipe_display[2]; // Pipe para comunicação entre processos
int sistema_pausado = 0; // Variavel para controlar pausa do sistema
int pausa_controle = 0; // Toggle para alternar entre pausa e continuação

// Função executada por cada thread esteira, simulando a contagem de produtos.
void* esteira(void* arg) {
    long id = (long)arg; // Identificador da esteira. (long devido bytes)
    double peso; // Peso do produto adicionado pela esteira.
    double intervalo; // Intervalo de tempo entre adições de produtos.

    // Define o peso e o intervalo de adição de produtos baseado no ID da esteira.
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
        usleep(intervalo * 1000000); // Espera pelo intervalo especificado antes de adicionar um novo produto.

        pthread_mutex_lock(&mutex_contagem); // Bloqueia o mutex para acesso exclusivo às variáveis compartilhadas.
        if (!sistema_pausado) { // Verifica se o sistema não está pausado.
            peso_esteira[id] += peso; // Adiciona o peso ao total da esteira.
            count[id]++; // Incrementa a contagem de produtos da esteira.

            // Verifica se o limite de produtos foi atingido para todas as esteiras juntas.
            if ((count[0] + count[1] + count[2]) % LIMITE_PRODUTOS == 0) {
                total_produtos += LIMITE_PRODUTOS;

                // Exibe os resultados acumulados até o momento.
                printf("\nAtualização dos valores:\n");
                for (int i = 0; i < NUM_THREADS; i++) {
                    printf("Esteira %d: Produtos = %d, Peso = %.2f kg\n", i + 1, count[i], peso_esteira[i]);
                }

                printf("\nLimite de %d produtos atingido. Total de peso das esteiras: %.2f kg\n", total_produtos, peso_esteira[0] + peso_esteira[1] + peso_esteira[2]);
                usleep(2 * 1000000); // Pausa por um curto período antes de continuar.
            }
        }
        pthread_mutex_unlock(&mutex_contagem); // Libera o mutex.
    }

    return NULL;
}

// Função de controle executada por uma thread para pausar/continuar o sistema.
void* controlador(void* arg) {
    printf("Pressione ENTER para pausar ou continuar o sistema.\n");

    while (1) {
        getchar(); // Aguarda o usuário pressionar ENTER.

        pthread_mutex_lock(&mutex_contagem); // Bloqueia o mutex para alterar o estado de pausa.
        if (!pausa_controle) {
            printf("Sistema pausado.\n");
            sistema_pausado = 1; // Ativa a pausa do sistema.
        } else {
            printf("Sistema continuando.\n");
            sistema_pausado = 0; // Desativa a pausa do sistema.
        }
        pausa_controle = !pausa_controle; // Alterna o estado de pausa.
        pthread_mutex_unlock(&mutex_contagem); // Libera o mutex.
    }

    return NULL;
}

// Função para exibir os resultados. É executada em um processo filho.
void exibe_resultados() {
    double peso[NUM_THREADS];
    int local_count[NUM_THREADS];

    while (1) {
        pthread_mutex_lock(&mutex_contagem); // Bloqueia o mutex para verificar se o sistema está pausado.
        if (sistema_pausado) {
            pthread_mutex_unlock(&mutex_contagem); // Se estiver pausado, libera o mutex e espera.
            usleep(1000000); // Espera ativamente se o sistema estiver pausado.
            continue; // Volta ao início do loop.
        }

        // Lê os dados do pipe apenas se o sistema não estiver pausado.
        read(pipe_display[0], &peso, sizeof(peso));
        read(pipe_display[0], &local_count, sizeof(local_count));
        pthread_mutex_unlock(&mutex_contagem); // Libera o mutex.

        // Exibe os resultados atuais.
        printf("\nAtualização dos valores:\n");
        for (int i = 0; i < NUM_THREADS; i++) {
            printf("Esteira %d: Produtos = %d, Peso = %.2f kg\n", i + 1, local_count[i], peso[i]);
        }

        usleep(INTERVALO_DISPLAY * 1000000); // Espera pelo intervalo de tempo definido antes de exibir novamente.
    }
}

// Função principal para configurar e iniciar o sistema.
int main() {
    pthread_t threads[NUM_THREADS]; // Armazena os identificadores das threads das esteiras.
    pthread_t thread_control; // Identificador da thread de controle.

    // Cria um pipe para comunicação entre processos.
    if (pipe(pipe_display) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork(); // Cria um processo filho.
    if (pid == 0) { 
        close(pipe_display[1]); // Fecha o lado de escrita do pipe.
        exibe_resultados(); // Chama a função para exibir os resultados.
        exit(0);
    } else if (pid > 0) { // Código executado pelo processo pai.
        close(pipe_display[0]); // Fecha o lado de leitura do pipe.

        // Cria threads para cada esteira e a thread de controle.
        for (long i = 0; i < NUM_THREADS; i++) {
            pthread_create(&threads[i], NULL, esteira, (void*)i);
        }
        pthread_create(&thread_control, NULL, controlador, NULL);

        // Loop principal para enviar dados atualizados para o processo filho.
        while (1) {
            pthread_mutex_lock(&mutex_contagem); // Bloqueia o mutex para acesso exclusivo.
            if (!sistema_pausado) { // Verifica se o sistema não está pausado.
                // Escreve os dados atualizados no pipe para o processo filho.
                write(pipe_display[1], &peso_esteira, sizeof(peso_esteira));
                write(pipe_display[1], &count, sizeof(count));
            }
            pthread_mutex_unlock(&mutex_contagem); // Libera o mutex.
            usleep(INTERVALO_DISPLAY * 1000000); // Espera pelo intervalo definido.
        }

        // Aguarda a conclusão das threads.
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }
        pthread_join(thread_control, NULL);

        wait(NULL); // Aguarda o processo filho terminar.
    } else { // Em caso de falha ao criar o processo filho.
        perror("fork");
        exit(EXIT_FAILURE);
    }

    return 0;
}
