/**
 * @file main.cpp
 * @author Gemini
 * @brief Implementação de uma simulação do algoritmo de substituição de páginas FIFO em C++23.
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este código simula o algoritmo First-In, First-Out (FIFO) para gerenciamento de páginas
 * de memória. Ele foi projetado para ser compilado com um compilador C++23 e otimizado
 * simbolicamente para o ambiente Windows 11, utilizando tipos de dados do Windows SDK.
 * A implementação não possui dependências externas além da Biblioteca Padrão C++ e do Windows SDK.
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
 * @brief Gerencia a substituição de páginas de memória usando o algoritmo FIFO.
 *
 * @details Esta classe encapsula a lógica de simulação do algoritmo FIFO. Ela mantém o estado
 * dos quadros de memória, a fila de páginas para substituição e as estatísticas de acertos (hits)
 * e falhas (page faults).
 */
class FifoPageManager {
public:
    /**
     * @brief Construtor da classe FifoPageManager.
     * @param total_frames O número total de quadros de memória física disponíveis na simulação.
     */
    explicit FifoPageManager(SIZE_T total_frames);

    /**
     * @brief Simula o acesso a uma página de memória.
     *
     * @details Este método é o núcleo da simulação. Ele verifica se a página solicitada
     * já está na memória (hit). Se não estiver (miss/page fault), ele a carrega,
     * substituindo a página mais antiga se a memória estiver cheia.
     *
     * @param page_number O número da página a ser acessada.
     */
    void accessPage(DWORD page_number);

    /**
     * @brief Imprime as estatísticas finais da simulação.
     *
     * @details Exibe o número total de acessos, page faults, hits e a taxa de acertos.
     */
    void printStatistics() const;

private:
    /**
     * @brief Imprime o estado atual dos quadros de memória.
     * @param result Uma string descrevendo o resultado do último acesso (HIT ou MISS).
     * @param victim_info Uma string opcional com informação sobre a página vítima.
     */
    void printFrames(const std::string& result, const std::string& victim_info = "") const;

    /// @brief O número total de quadros de memória física.
    const SIZE_T num_frames;

    /// @brief Vetor que representa os quadros de memória física. Um quadro pode conter uma página ou estar vazio.
    std::vector<std::optional<DWORD>> frames;

    /// @brief Tabela de páginas para busca rápida. Mapeia um número de página para o índice do quadro onde está armazenada.
    std::unordered_map<DWORD, SIZE_T> page_table;

    /// @brief Fila que mantém a ordem de entrada das páginas para a política de substituição FIFO.
    std::queue<DWORD> fifo_queue;

    /// @brief Contador de falhas de página (page faults/misses).
    SIZE_T page_faults;

    /// @brief Contador de acertos de página (hits).
    SIZE_T hits;
};

FifoPageManager::FifoPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    page_faults(0),
    hits(0) {
    frames.resize(num_frames, std::nullopt); // Inicializa os quadros como vazios.
}

void FifoPageManager::accessPage(DWORD page_number) {
    std::cout << std::format("Página {:>2} | ", page_number);

    // 1. Acesso à página: Verificar se a página está na memória (HIT)
    if (page_table.contains(page_number)) {
        hits++;
        printFrames("HIT");
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Se ausente e há quadro livre
    if (page_table.size() < num_frames) {
        SIZE_T free_frame_index = 0;
        // Encontra o primeiro quadro livre
        for (SIZE_T i = 0; i < num_frames; ++i) {
            if (!frames[i].has_value()) {
                free_frame_index = i;
                break;
            }
        }

        // Carrega a nova página no quadro livre
        frames[free_frame_index] = page_number;
        page_table[page_number] = free_frame_index;
        fifo_queue.push(page_number);
        printFrames("MISS");
    }
    else {
        // 4. Se ausente e memória cheia:
        // Selecionar vítima usando o ponteiro FIFO (o primeiro da fila)
        DWORD victim_page = fifo_queue.front();
        fifo_queue.pop();

        // Encontrar o quadro da página vítima
        SIZE_T frame_index_to_replace = page_table.at(victim_page);

        // Remover a página vítima da tabela de páginas
        page_table.erase(victim_page);

        // Carregar a nova página no quadro liberado
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

    std::cout << "\n--- Estatísticas Finais ---\n";
    std::cout << std::format("Total de Acessos:  {}\n", total_accesses);
    std::cout << std::format("Page Faults (Miss):{}\n", page_faults);
    std::cout << std::format("Hits:              {}\n", hits);
    std::cout << std::format("Taxa de Acertos:   {:.2f}%\n", hit_ratio);
    std::cout << "---------------------------\n";
}

/**
 * @brief Função principal que executa a simulação.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    // Define a localidade para pt-BR para garantir a formatação correta de números.
    std::locale::global(std::locale("pt_BR.UTF-8"));

    // Exemplo prático do texto de referência
    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simulação FIFO com " << NUM_FRAMES << " quadros.\n\n";

    FifoPageManager manager(NUM_FRAMES);

    for (const auto& page : reference_string) {
        manager.accessPage(page);
    }

    manager.printStatistics();

    return 0;
}