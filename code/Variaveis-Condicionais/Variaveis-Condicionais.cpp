/**
 * @file main.cpp
 * @author Gemini Assistant
 * @date 19 Aug 2025
 * @brief Exemplo completo do padr�o Produtor-Consumidor usando std::condition_variable em C++23.
 *
 * @details Este programa demonstra a sincroniza��o entre duas threads, uma produtora
 * e uma consumidora, que se comunicam atrav�s de uma fila compartilhada. A sincroniza��o
 * � gerenciada usando um mutex para exclus�o m�tua e uma vari�vel de condi��o para
 * uma espera eficiente.
 * Este c�digo foi projetado para ser compilado com o compilador MSVC no Visual Studio 2022,
 * com o padr�o da linguagem configurado para C++23.
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
  * @brief Mutex para proteger o acesso � fila de dados e � flag de finaliza��o.
  * @details Garante que apenas uma thread possa modificar os recursos compartilhados por vez,
  * prevenindo condi��es de corrida.
  */
std::mutex mtx;

/**
 * @brief Vari�vel de condi��o para sincronizar produtor e consumidor.
 * @details Permite que a thread consumidora espere eficientemente (sem gastar CPU)
 * at� que o produtor coloque um novo item na fila ou sinalize o fim da produ��o.
 */
std::condition_variable cv;

/**
 * @brief Fila (queue) que serve como buffer compartilhado entre as threads.
 * @details O produtor insere itens nesta fila e o consumidor os remove.
 */
std::queue<int> data_queue;

/**
 * @brief Flag booleana para sinalizar ao consumidor que a produ��o terminou.
 * @details � essencial para que o consumidor saiba quando pode parar de esperar por novos
 * itens e encerrar sua execu��o de forma limpa.
 */
bool production_finished = false;


/**
 * @brief A fun��o da thread produtora.
 * @details Gera uma sequ�ncia de 10 n�meros inteiros, com um pequeno atraso entre cada um,
 * e os insere na fila compartilhada. Ao final, sinaliza que a produ��o terminou.
 */
void producer() {
    std::cout << "[PRODUTOR] Iniciando a produ��o...\n";
    for (int i = 0; i < 10; ++i) {
        // Simula um trabalho de produ��o de dados
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        {
            // Adquire o lock para modificar os recursos compartilhados.
            // std::lock_guard garante que o mutex ser� liberado ao sair do escopo.
            std::lock_guard<std::mutex> lock(mtx);

            std::cout << "[PRODUTOR] Produziu o item: " << i << "\n";
            data_queue.push(i);
        } // O lock � liberado aqui, quando 'lock' � destru�do.

        // Notifica UMA thread que est� esperando (a consumidora) que uma condi��o mudou.
        // � importante notificar AP�S liberar o lock para evitar que a thread acordada
        // tente adquirir o lock e seja bloqueada imediatamente.
        cv.notify_one();
    }

    // Ap�s terminar o loop, adquire o lock novamente para sinalizar o fim.
    {
        std::lock_guard<std::mutex> lock(mtx);
        production_finished = true;
        std::cout << "[PRODUTOR] Produ��o finalizada.\n";
    }
    cv.notify_one(); // Notifica o consumidor uma �ltima vez, caso ele esteja esperando.
}


/**
 * @brief A fun��o da thread consumidora.
 * @details Espera at� que a fila de dados n�o esteja vazia. Quando acordada, consome um item
 * da fila. Continua executando at� que a produ��o tenha terminado e a fila esteja vazia.
 */
void consumer() {
    std::cout << "[CONSUMIDOR] Aguardando por itens...\n";
    while (true) {
        // std::unique_lock � necess�rio para a vari�vel de condi��o, pois ela precisa
        // da flexibilidade de ser destravada durante a espera e retravada ao acordar.
        std::unique_lock<std::mutex> lock(mtx);

        // A chamada wait() faz 3 coisas atomicamente:
        // 1. Libera o lock.
        // 2. Coloca a thread para dormir.
        // 3. Quando notificada (ou por um despertar esp�rio), acorda e tenta readquirir o lock.
        // O segundo argumento (um lambda) � a condi��o de espera. A thread s� prosseguir�
        // quando a condi��o for verdadeira. Isso protege contra "despertares esp�rios".
        cv.wait(lock, [] { return !data_queue.empty() || production_finished; });

        // Neste ponto, a thread est� acordada E possui o lock.

        if (!data_queue.empty()) {
            int item = data_queue.front();
            data_queue.pop();

            // � uma boa pr�tica liberar o lock o mais r�pido poss�vel,
            // especialmente se o processamento do item for demorado.
            lock.unlock();

            std::cout << "[CONSUMIDOR] Consumiu o item: " << item << "\n";
            // Simula um trabalho de processamento do item
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

        }
        else if (production_finished) {
            // A fila est� vazia e o produtor terminou. Podemos sair do loop.
            std::cout << "[CONSUMIDOR] Produ��o finalizada e fila vazia. Encerrando.\n";
            break;
        }
    }
}


/**
 * @brief Ponto de entrada principal do programa.
 * @details Cria e inicia as threads produtora e consumidora, e espera que ambas
 * terminem sua execu��o antes de finalizar o programa.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    std::cout << "Iniciando exemplo de Produtor-Consumidor com Variavel de Condicao.\n";
    std::cout << "-----------------------------------------------------------------\n";

    // Cria e inicia as threads
    std::thread producer_thread(producer);
    std::thread consumer_thread(consumer);

    // Espera que as threads terminem sua execu��o.
    // Isso � essencial para garantir que o programa n�o termine prematuramente.
    producer_thread.join();
    consumer_thread.join();

    std::cout << "-----------------------------------------------------------------\n";
    std::cout << "Programa finalizado com sucesso.\n";

    return 0;
}