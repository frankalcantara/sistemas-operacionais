/**
 * @file main.cpp
 * @author Gemini Assistant
 * @date 19 Aug 2025
 * @brief Exemplo completo do padrão Produtor-Consumidor usando std::condition_variable em C++23.
 *
 * @details Este programa demonstra a sincronização entre duas threads, uma produtora
 * e uma consumidora, que se comunicam através de uma fila compartilhada. A sincronização
 * é gerenciada usando um mutex para exclusão mútua e uma variável de condição para
 * uma espera eficiente.
 * Este código foi projetado para ser compilado com o compilador MSVC no Visual Studio 2022,
 * com o padrão da linguagem configurado para C++23.
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <string>

 // --- Recursos Compartilhados ---

 /**
  * @brief Mutex para proteger o acesso à fila de dados e à flag de finalização.
  * @details Garante que apenas uma thread possa modificar os recursos compartilhados por vez,
  * prevenindo condições de corrida.
  */
std::mutex mtx;

/**
 * @brief Variável de condição para sincronizar produtor e consumidor.
 * @details Permite que a thread consumidora espere eficientemente (sem gastar CPU)
 * até que o produtor coloque um novo item na fila ou sinalize o fim da produção.
 */
std::condition_variable cv;

/**
 * @brief Fila (queue) que serve como buffer compartilhado entre as threads.
 * @details O produtor insere itens nesta fila e o consumidor os remove.
 */
std::queue<int> data_queue;

/**
 * @brief Flag booleana para sinalizar ao consumidor que a produção terminou.
 * @details É essencial para que o consumidor saiba quando pode parar de esperar por novos
 * itens e encerrar sua execução de forma limpa.
 */
bool production_finished = false;


/**
 * @brief A função da thread produtora.
 * @details Gera uma sequência de 10 números inteiros, com um pequeno atraso entre cada um,
 * e os insere na fila compartilhada. Ao final, sinaliza que a produção terminou.
 */
void producer() {
    std::cout << "[PRODUTOR] Iniciando a produção...\n";
    for (int i = 0; i < 10; ++i) {
        // Simula um trabalho de produção de dados
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        {
            // Adquire o lock para modificar os recursos compartilhados.
            // std::lock_guard garante que o mutex será liberado ao sair do escopo.
            std::lock_guard<std::mutex> lock(mtx);

            std::cout << "[PRODUTOR] Produziu o item: " << i << "\n";
            data_queue.push(i);
        } // O lock é liberado aqui, quando 'lock' é destruído.

        // Notifica UMA thread que está esperando (a consumidora) que uma condição mudou.
        // É importante notificar APÓS liberar o lock para evitar que a thread acordada
        // tente adquirir o lock e seja bloqueada imediatamente.
        cv.notify_one();
    }

    // Após terminar o loop, adquire o lock novamente para sinalizar o fim.
    {
        std::lock_guard<std::mutex> lock(mtx);
        production_finished = true;
        std::cout << "[PRODUTOR] Produção finalizada.\n";
    }
    cv.notify_one(); // Notifica o consumidor uma última vez, caso ele esteja esperando.
}


/**
 * @brief A função da thread consumidora.
 * @details Espera até que a fila de dados não esteja vazia. Quando acordada, consome um item
 * da fila. Continua executando até que a produção tenha terminado e a fila esteja vazia.
 */
void consumer() {
    std::cout << "[CONSUMIDOR] Aguardando por itens...\n";
    while (true) {
        // std::unique_lock é necessário para a variável de condição, pois ela precisa
        // da flexibilidade de ser destravada durante a espera e retravada ao acordar.
        std::unique_lock<std::mutex> lock(mtx);

        // A chamada wait() faz 3 coisas atomicamente:
        // 1. Libera o lock.
        // 2. Coloca a thread para dormir.
        // 3. Quando notificada (ou por um despertar espúrio), acorda e tenta readquirir o lock.
        // O segundo argumento (um lambda) é a condição de espera. A thread só prosseguirá
        // quando a condição for verdadeira. Isso protege contra "despertares espúrios".
        cv.wait(lock, [] { return !data_queue.empty() || production_finished; });

        // Neste ponto, a thread está acordada E possui o lock.

        if (!data_queue.empty()) {
            int item = data_queue.front();
            data_queue.pop();

            // É uma boa prática liberar o lock o mais rápido possível,
            // especialmente se o processamento do item for demorado.
            lock.unlock();

            std::cout << "[CONSUMIDOR] Consumiu o item: " << item << "\n";
            // Simula um trabalho de processamento do item
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

        }
        else if (production_finished) {
            // A fila está vazia e o produtor terminou. Podemos sair do loop.
            std::cout << "[CONSUMIDOR] Produção finalizada e fila vazia. Encerrando.\n";
            break;
        }
    }
}


/**
 * @brief Ponto de entrada principal do programa.
 * @details Cria e inicia as threads produtora e consumidora, e espera que ambas
 * terminem sua execução antes de finalizar o programa.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    std::cout << "Iniciando exemplo de Produtor-Consumidor com Variavel de Condicao.\n";
    std::cout << "-----------------------------------------------------------------\n";

    // Cria e inicia as threads
    std::thread producer_thread(producer);
    std::thread consumer_thread(consumer);

    // Espera que as threads terminem sua execução.
    // Isso é essencial para garantir que o programa não termine prematuramente.
    producer_thread.join();
    consumer_thread.join();

    std::cout << "-----------------------------------------------------------------\n";
    std::cout << "Programa finalizado com sucesso.\n";

    return 0;
}