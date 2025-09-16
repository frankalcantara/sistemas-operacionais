/**
 * @file main.cpp
 * @author Gemini
 * @brief Implementa��o de uma simula��o do algoritmo de substitui��o de p�ginas FIFO em C++23.
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este c�digo simula o algoritmo First-In, First-Out (FIFO) para gerenciamento de p�ginas
 * de mem�ria. Ele foi projetado para ser compilado com um compilador C++23 e otimizado
 * simbolicamente para o ambiente Windows 11, utilizando tipos de dados do Windows SDK.
 * A implementa��o n�o possui depend�ncias externas al�m da Biblioteca Padr�o C++ e do Windows SDK.
 */

#include <iostream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <optional>
#include <string>
#include <format>

 // Inclui o SDK do Windows para tipos de dados nativos como DWORD e SIZE_T
#include <Windows.h>

/**
 * @class FifoPageManager
 * @brief Gerencia a substitui��o de p�ginas de mem�ria usando o algoritmo FIFO.
 *
 * @details Esta classe encapsula a l�gica de simula��o do algoritmo FIFO. Ela mant�m o estado
 * dos quadros de mem�ria, a fila de p�ginas para substitui��o e as estat�sticas de acertos (hits)
 * e falhas (page faults).
 */
class FifoPageManager {
public:
    /**
     * @brief Construtor da classe FifoPageManager.
     * @param total_frames O n�mero total de quadros de mem�ria f�sica dispon�veis na simula��o.
     */
    explicit FifoPageManager(SIZE_T total_frames);

    /**
     * @brief Simula o acesso a uma p�gina de mem�ria.
     *
     * @details Este m�todo � o n�cleo da simula��o. Ele verifica se a p�gina solicitada
     * j� est� na mem�ria (hit). Se n�o estiver (miss/page fault), ele a carrega,
     * substituindo a p�gina mais antiga se a mem�ria estiver cheia.
     *
     * @param page_number O n�mero da p�gina a ser acessada.
     */
    void accessPage(DWORD page_number);

    /**
     * @brief Imprime as estat�sticas finais da simula��o.
     *
     * @details Exibe o n�mero total de acessos, page faults, hits e a taxa de acertos.
     */
    void printStatistics() const;

private:
    /**
     * @brief Imprime o estado atual dos quadros de mem�ria.
     * @param result Uma string descrevendo o resultado do �ltimo acesso (HIT ou MISS).
     * @param victim_info Uma string opcional com informa��o sobre a p�gina v�tima.
     */
    void printFrames(const std::string& result, const std::string& victim_info = "") const;

    /// @brief O n�mero total de quadros de mem�ria f�sica.
    const SIZE_T num_frames;

    /// @brief Vetor que representa os quadros de mem�ria f�sica. Um quadro pode conter uma p�gina ou estar vazio.
    std::vector<std::optional<DWORD>> frames;

    /// @brief Tabela de p�ginas para busca r�pida. Mapeia um n�mero de p�gina para o �ndice do quadro onde est� armazenada.
    std::unordered_map<DWORD, SIZE_T> page_table;

    /// @brief Fila que mant�m a ordem de entrada das p�ginas para a pol�tica de substitui��o FIFO.
    std::queue<DWORD> fifo_queue;

    /// @brief Contador de falhas de p�gina (page faults/misses).
    SIZE_T page_faults;

    /// @brief Contador de acertos de p�gina (hits).
    SIZE_T hits;
};

FifoPageManager::FifoPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    page_faults(0),
    hits(0) {
    frames.resize(num_frames, std::nullopt); // Inicializa os quadros como vazios.
}

void FifoPageManager::accessPage(DWORD page_number) {
    std::cout << std::format("P�gina {:>2} | ", page_number);

    // 1. Acesso � p�gina: Verificar se a p�gina est� na mem�ria (HIT)
    if (page_table.contains(page_number)) {
        hits++;
        printFrames("HIT");
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Se ausente e h� quadro livre
    if (page_table.size() < num_frames) {
        SIZE_T free_frame_index = 0;
        // Encontra o primeiro quadro livre
        for (SIZE_T i = 0; i < num_frames; ++i) {
            if (!frames[i].has_value()) {
                free_frame_index = i;
                break;
            }
        }

        // Carrega a nova p�gina no quadro livre
        frames[free_frame_index] = page_number;
        page_table[page_number] = free_frame_index;
        fifo_queue.push(page_number);
        printFrames("MISS");
    }
    else {
        // 4. Se ausente e mem�ria cheia:
        // Selecionar v�tima usando o ponteiro FIFO (o primeiro da fila)
        DWORD victim_page = fifo_queue.front();
        fifo_queue.pop();

        // Encontrar o quadro da p�gina v�tima
        SIZE_T frame_index_to_replace = page_table.at(victim_page);

        // Remover a p�gina v�tima da tabela de p�ginas
        page_table.erase(victim_page);

        // Carregar a nova p�gina no quadro liberado
        frames[frame_index_to_replace] = page_number;
        page_table[page_number] = frame_index_to_replace;
        fifo_queue.push(page_number);

        std::string victim_info = std::format("(remove {})", victim_page);
        printFrames("MISS", victim_info);
    }
}

void FifoPageManager::printFrames(const std::string& result, const std::string& victim_info) const {
    std::cout << "Quadros [";
    for (SIZE_T i = 0; i < frames.size(); ++i) {
        if (frames[i].has_value()) {
            std::cout << *frames[i];
        }
        else {
            std::cout << "-";
        }
        if (i < frames.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::format("] | {} {}", result, victim_info) << std::endl;
}

void FifoPageManager::printStatistics() const {
    const SIZE_T total_accesses = hits + page_faults;
    const double hit_ratio = (total_accesses > 0) ? (static_cast<double>(hits) / total_accesses) * 100.0 : 0.0;

    std::cout << "\n--- Estat�sticas Finais ---\n";
    std::cout << std::format("Total de Acessos:  {}\n", total_accesses);
    std::cout << std::format("Page Faults (Miss):{}\n", page_faults);
    std::cout << std::format("Hits:              {}\n", hits);
    std::cout << std::format("Taxa de Acertos:   {:.2f}%\n", hit_ratio);
    std::cout << "---------------------------\n";
}

/**
 * @brief Fun��o principal que executa a simula��o.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    // Define a localidade para pt-BR para garantir a formata��o correta de n�meros.
    std::locale::global(std::locale("pt_BR.UTF-8"));

    // Exemplo pr�tico do texto de refer�ncia
    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simula��o FIFO com " << NUM_FRAMES << " quadros.\n\n";

    FifoPageManager manager(NUM_FRAMES);

    for (const auto& page : reference_string) {
        manager.accessPage(page);
    }

    manager.printStatistics();

    return 0;
}