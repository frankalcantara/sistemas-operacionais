/**
 * @file main_opt.cpp
 * @author Frank Alcantara
 * @brief Implementa��o de uma simula��o do algoritmo de substitui��o de p�ginas �timo (OPT/MIN).
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este c�digo simula o algoritmo �timo (tamb�m conhecido como MIN ou de Belady)
 * para gerenciamento de p�ginas de mem�ria. Ele serve como um benchmark te�rico, pois requer
 * conhecimento pr�vio de toda a sequ�ncia de acessos a p�ginas. O c�digo foi projetado
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
 * @brief Gerencia a substitui��o de p�ginas de mem�ria usando o algoritmo �timo (OPT/MIN).
 *
 * @details Esta classe encapsula a l�gica de simula��o do algoritmo �timo. Para cada falha de p�gina
 * com a mem�ria cheia, ela analisa os acessos futuros para escolher a melhor p�gina a ser
 * substitu�da: aquela que n�o ser� usada por mais tempo.
 */
class OptimalPageManager {
public:
    /**
     * @brief Construtor da classe OptimalPageManager.
     * @param total_frames O n�mero total de quadros de mem�ria f�sica dispon�veis na simula��o.
     */
    explicit OptimalPageManager(SIZE_T total_frames);

    /**
     * @brief Executa a simula��o completa para uma dada string de refer�ncia.
     * @param reference_string O vetor de p�ginas a serem acessadas sequencialmente.
     */
    void simulate(const std::vector<DWORD>& reference_string);

    /**
     * @brief Imprime as estat�sticas finais da simula��o.
     * @details Exibe o n�mero total de acessos, page faults, hits e a taxa de acertos.
     */
    void printStatistics() const;

private:
    /**
     * @brief Simula o acesso a uma �nica p�gina de mem�ria, considerando os acessos futuros.
     * @param reference_string A string de refer�ncia completa.
     * @param current_index O �ndice do acesso atual na string de refer�ncia.
     */
    void accessPage(const std::vector<DWORD>& reference_string, SIZE_T current_index);

    /**
     * @brief Imprime o estado atual dos quadros de mem�ria.
     * @param page O n�mero da p�gina sendo acessada.
     * @param result Uma string descrevendo o resultado do acesso (HIT ou MISS).
     * @param analysis Uma string opcional com a an�lise futura ou informa��o da v�tima.
     */
    void printFrames(DWORD page, const std::string& result, const std::string& analysis = "") const;

    /// @brief O n�mero total de quadros de mem�ria f�sica.
    const SIZE_T num_frames;

    /// @brief Vetor que representa os quadros de mem�ria f�sica.
    std::vector<std::optional<DWORD>> frames;

    /// @brief Tabela de p�ginas para busca r�pida (Page Number -> Frame Index).
    std::unordered_map<DWORD, SIZE_T> page_table;

    /// @brief Contador de falhas de p�gina (page faults/misses).
    SIZE_T page_faults;

    /// @brief Contador de acertos de p�gina (hits).
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

    // 1. Acesso � p�gina: Verificar se a p�gina est� na mem�ria (HIT)
    if (page_table.contains(page_number)) {
        hits++;
        printFrames(page_number, "HIT");
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Se ausente e h� quadro livre
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
        // 4. Se ausente e mem�ria cheia: executar o algoritmo �timo
        DWORD victim_page = -1;
        SIZE_T farthest_future_use = current_index;
        std::string analysis_log = "An�lise: ";

        // Para cada p�gina atualmente nos quadros, encontrar seu pr�ximo uso
        for (const auto& entry : page_table) {
            const DWORD current_page_in_frame = entry.first;
            auto future_it = std::find(reference_string.begin() + current_index + 1, reference_string.end(), current_page_in_frame);

            // Se a p�gina n�o for mais usada no futuro, ela � a v�tima perfeita
            if (future_it == reference_string.end()) {
                victim_page = current_page_in_frame;
                analysis_log += std::format("{}->nunca", current_page_in_frame);
                break;
            }

            // Caso contr�rio, verifica se seu uso � o mais distante at� agora
            SIZE_T future_pos = std::distance(reference_string.begin(), future_it);
            analysis_log += std::format("{}->pos.{}, ", current_page_in_frame, future_pos);
            if (future_pos > farthest_future_use) {
                farthest_future_use = future_pos;
                victim_page = current_page_in_frame;
            }
        }

        // Remove a �ltima v�rgula e espa�o da an�lise
        if (analysis_log.ends_with(", ")) {
            analysis_log.pop_back();
            analysis_log.pop_back();
        }

        // Substituir a p�gina v�tima
        SIZE_T frame_to_replace = page_table.at(victim_page);
        page_table.erase(victim_page);

        frames[frame_to_replace] = page_number;
        page_table[page_number] = frame_to_replace;

        std::string final_analysis = std::format("{} (remove {})", analysis_log, victim_page);
        printFrames(page_number, "MISS", final_analysis);
    }
}

void OptimalPageManager::printFrames(DWORD page, const std::string& result, const std::string& analysis) const {
    std::cout << std::format("P�gina {:>2} | Quadros [", page);
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

    std::cout << "\n--- Estat�sticas Finais (�timo/MIN) ---\n";
    std::cout << std::format("Total de Acessos:  {}\n", total_accesses);
    std::cout << std::format("Page Faults (Miss):{}\n", page_faults);
    std::cout << std::format("Hits:              {}\n", hits);
    std::cout << std::format("Taxa de Acertos:   {:.2f}%\n", hit_ratio);
    std::cout << "--------------------------------------\n";
}

/**
 * @brief Fun��o principal que executa a simula��o do algoritmo �timo.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    std::locale::global(std::locale("pt_BR.UTF-8"));

    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simula��o �tima (MIN) com " << NUM_FRAMES << " quadros.\n\n";

    OptimalPageManager manager(NUM_FRAMES);
    manager.simulate(reference_string);
    manager.printStatistics();

    return 0;
}