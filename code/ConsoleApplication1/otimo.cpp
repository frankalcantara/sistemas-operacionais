/**
 * @file main_opt.cpp
 * @author Frank Alcantara
 * @brief Implementação de uma simulação do algoritmo de substituição de páginas Ótimo (OPT/MIN).
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este código simula o algoritmo Ótimo (também conhecido como MIN ou de Belady)
 * para gerenciamento de páginas de memória. Ele serve como um benchmark teórico, pois requer
 * conhecimento prévio de toda a sequência de acessos a páginas. O código foi projetado
 * para C++23 e utiliza tipos de dados do Windows SDK.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>
#include <format>
#include <limits>
#include <algorithm> // Para std::find

 // Inclui o SDK do Windows para tipos de dados nativos como DWORD e SIZE_T
#include <Windows.h>

/**
 * @class OptimalPageManager
 * @brief Gerencia a substituição de páginas de memória usando o algoritmo Ótimo (OPT/MIN).
 *
 * @details Esta classe encapsula a lógica de simulação do algoritmo Ótimo. Para cada falha de página
 * com a memória cheia, ela analisa os acessos futuros para escolher a melhor página a ser
 * substituída: aquela que não será usada por mais tempo.
 */
class OptimalPageManager {
public:
    /**
     * @brief Construtor da classe OptimalPageManager.
     * @param total_frames O número total de quadros de memória física disponíveis na simulação.
     */
    explicit OptimalPageManager(SIZE_T total_frames);

    /**
     * @brief Executa a simulação completa para uma dada string de referência.
     * @param reference_string O vetor de páginas a serem acessadas sequencialmente.
     */
    void simulate(const std::vector<DWORD>& reference_string);

    /**
     * @brief Imprime as estatísticas finais da simulação.
     * @details Exibe o número total de acessos, page faults, hits e a taxa de acertos.
     */
    void printStatistics() const;

private:
    /**
     * @brief Simula o acesso a uma única página de memória, considerando os acessos futuros.
     * @param reference_string A string de referência completa.
     * @param current_index O índice do acesso atual na string de referência.
     */
    void accessPage(const std::vector<DWORD>& reference_string, SIZE_T current_index);

    /**
     * @brief Imprime o estado atual dos quadros de memória.
     * @param page O número da página sendo acessada.
     * @param result Uma string descrevendo o resultado do acesso (HIT ou MISS).
     * @param analysis Uma string opcional com a análise futura ou informação da vítima.
     */
    void printFrames(DWORD page, const std::string& result, const std::string& analysis = "") const;

    /// @brief O número total de quadros de memória física.
    const SIZE_T num_frames;

    /// @brief Vetor que representa os quadros de memória física.
    std::vector<std::optional<DWORD>> frames;

    /// @brief Tabela de páginas para busca rápida (Page Number -> Frame Index).
    std::unordered_map<DWORD, SIZE_T> page_table;

    /// @brief Contador de falhas de página (page faults/misses).
    SIZE_T page_faults;

    /// @brief Contador de acertos de página (hits).
    SIZE_T hits;
};

OptimalPageManager::OptimalPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    page_faults(0),
    hits(0) {
    frames.resize(num_frames, std::nullopt);
}

void OptimalPageManager::simulate(const std::vector<DWORD>& reference_string) {
    for (SIZE_T i = 0; i < reference_string.size(); ++i) {
        accessPage(reference_string, i);
    }
}

void OptimalPageManager::accessPage(const std::vector<DWORD>& reference_string, SIZE_T current_index) {
    const DWORD page_number = reference_string[current_index];

    // 1. Acesso à página: Verificar se a página está na memória (HIT)
    if (page_table.contains(page_number)) {
        hits++;
        printFrames(page_number, "HIT");
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Se ausente e há quadro livre
    if (page_table.size() < num_frames) {
        SIZE_T free_frame_index = 0;
        for (SIZE_T i = 0; i < num_frames; ++i) {
            if (!frames[i].has_value()) {
                free_frame_index = i;
                break;
            }
        }
        frames[free_frame_index] = page_number;
        page_table[page_number] = free_frame_index;
        printFrames(page_number, "MISS");
    }
    else {
        // 4. Se ausente e memória cheia: executar o algoritmo Ótimo
        DWORD victim_page = -1;
        SIZE_T farthest_future_use = current_index;
        std::string analysis_log = "Análise: ";

        // Para cada página atualmente nos quadros, encontrar seu próximo uso
        for (const auto& entry : page_table) {
            const DWORD current_page_in_frame = entry.first;
            auto future_it = std::find(reference_string.begin() + current_index + 1, reference_string.end(), current_page_in_frame);

            // Se a página não for mais usada no futuro, ela é a vítima perfeita
            if (future_it == reference_string.end()) {
                victim_page = current_page_in_frame;
                analysis_log += std::format("{}->nunca", current_page_in_frame);
                break;
            }

            // Caso contrário, verifica se seu uso é o mais distante até agora
            SIZE_T future_pos = std::distance(reference_string.begin(), future_it);
            analysis_log += std::format("{}->pos.{}, ", current_page_in_frame, future_pos);
            if (future_pos > farthest_future_use) {
                farthest_future_use = future_pos;
                victim_page = current_page_in_frame;
            }
        }

        // Remove a última vírgula e espaço da análise
        if (analysis_log.ends_with(", ")) {
            analysis_log.pop_back();
            analysis_log.pop_back();
        }

        // Substituir a página vítima
        SIZE_T frame_to_replace = page_table.at(victim_page);
        page_table.erase(victim_page);

        frames[frame_to_replace] = page_number;
        page_table[page_number] = frame_to_replace;

        std::string final_analysis = std::format("{} (remove {})", analysis_log, victim_page);
        printFrames(page_number, "MISS", final_analysis);
    }
}

void OptimalPageManager::printFrames(DWORD page, const std::string& result, const std::string& analysis) const {
    std::cout << std::format("Página {:>2} | Quadros [", page);
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
    std::cout << "] | " << result;
    if (!analysis.empty()) {
        std::cout << " | " << analysis;
    }
    std::cout << std::endl;
}

void OptimalPageManager::printStatistics() const {
    const SIZE_T total_accesses = hits + page_faults;
    const double hit_ratio = (total_accesses > 0) ? (static_cast<double>(hits) / total_accesses) * 100.0 : 0.0;

    std::cout << "\n--- Estatísticas Finais (Ótimo/MIN) ---\n";
    std::cout << std::format("Total de Acessos:  {}\n", total_accesses);
    std::cout << std::format("Page Faults (Miss):{}\n", page_faults);
    std::cout << std::format("Hits:              {}\n", hits);
    std::cout << std::format("Taxa de Acertos:   {:.2f}%\n", hit_ratio);
    std::cout << "--------------------------------------\n";
}

/**
 * @brief Função principal que executa a simulação do algoritmo Ótimo.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    std::locale::global(std::locale("pt_BR.UTF-8"));

    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simulação Ótima (MIN) com " << NUM_FRAMES << " quadros.\n\n";

    OptimalPageManager manager(NUM_FRAMES);
    manager.simulate(reference_string);
    manager.printStatistics();

    return 0;
}