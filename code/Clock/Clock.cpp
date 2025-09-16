/**
 * @file main_clock.cpp
 * @author Gemini
 * @brief Implementa��o compat�vel do algoritmo de substitui��o de p�ginas Rel�gio (Clock/Second Chance).
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este c�digo simula o algoritmo do Rel�gio, uma aproxima��o eficiente do LRU.
 * Ele utiliza um bit de refer�ncia e um ponteiro circular para dar uma "segunda chance"
 * �s p�ginas antes de substitu�-las. Esta vers�o usa iostream/iomanip para m�xima
 * compatibilidade de compiladores.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>
#include <iomanip>

 // Inclui o SDK do Windows para tipos de dados nativos como DWORD e SIZE_T
#include <Windows.h>

/**
 * @class ClockPageManager
 * @brief Gerencia a substitui��o de p�ginas de mem�ria usando o algoritmo do Rel�gio.
 *
 * @details A classe simula uma lista circular de quadros, cada um com um bit de refer�ncia.
 * Um ponteiro (clock_hand) varre os quadros em busca de uma p�gina com bit de refer�ncia
 * zero para substituir.
 */
class ClockPageManager {
public:
    explicit ClockPageManager(SIZE_T total_frames);
    void accessPage(DWORD page_number);
    void printStatistics() const;

private:
    void printFrames(const std::string& result, const std::string& extra_info = "") const;

    /// @brief O n�mero total de quadros de mem�ria f�sica.
    const SIZE_T num_frames;

    /// @brief Vetor que representa os quadros de mem�ria f�sica.
    std::vector<std::optional<DWORD>> frames;

    /// @brief Simula os bits de refer�ncia (R-bits) para cada quadro.
    std::vector<bool> reference_bits;

    /// @brief Tabela de p�ginas para busca r�pida (Page Number -> Frame Index).
    std::unordered_map<DWORD, SIZE_T> page_table;

    /// @brief O ponteiro do rel�gio, que aponta para o pr�ximo candidato � substitui��o.
    SIZE_T clock_hand;

    /// @brief Contador de falhas de p�gina.
    SIZE_T page_faults;

    /// @brief Contador de acertos de p�gina.
    SIZE_T hits;
};

ClockPageManager::ClockPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    clock_hand(0),
    page_faults(0),
    hits(0) {
    frames.resize(num_frames, std::nullopt);
    reference_bits.resize(num_frames, false);
}

void ClockPageManager::accessPage(DWORD page_number) {
    std::cout << "P�gina " << std::setw(2) << page_number << " | ";

    // 1. Verificar se a p�gina est� na mem�ria (HIT)
    auto it = page_table.find(page_number);
    if (it != page_table.end()) {
        hits++;
        // Em um HIT, o hardware define o bit de refer�ncia como 1
        reference_bits[it->second] = true;
        printFrames("HIT");
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Tenta encontrar um quadro livre primeiro (fase inicial)
    // Embora o algoritmo puro use o ponteiro, na pr�tica, quadros vazios s�o preenchidos primeiro.
    std::optional<SIZE_T> free_frame;
    for (SIZE_T i = 0; i < num_frames; ++i) {
        if (!frames[i].has_value()) {
            free_frame = i;
            break;
        }
    }

    if (free_frame.has_value()) {
        SIZE_T frame_index = *free_frame;
        frames[frame_index] = page_number;
        reference_bits[frame_index] = true; // P�gina nova entra com bit 1
        page_table[page_number] = frame_index;
        printFrames("MISS");
    }
    else {
        // 4. Se a mem�ria est� cheia, executar o algoritmo do Rel�gio para encontrar a v�tima
        while (true) {
            // a. Examinar o quadro atual do ponteiro
            if (reference_bits[clock_hand] == false) {
                // b. Se bit = 0: selecionar como v�tima
                const DWORD victim_page = frames[clock_hand].value();

                // Remove a v�tima e insere a nova p�gina
                page_table.erase(victim_page);
                frames[clock_hand] = page_number;
                reference_bits[clock_hand] = true; // P�gina nova entra com bit 1
                page_table[page_number] = clock_hand;

                std::string miss_info = "(remove ";
                miss_info += std::to_string(victim_page);
                miss_info += ")";

                // Avan�a o ponteiro para a pr�xima posi��o para o pr�ximo ciclo
                clock_hand = (clock_hand + 1) % num_frames;

                printFrames("MISS", miss_info);
                break; // V�tima encontrada, sai do loop
            }
            else {
                // c. Se bit = 1: dar segunda chance (zerar o bit) e avan�ar
                reference_bits[clock_hand] = false;
                clock_hand = (clock_hand + 1) % num_frames;
            }
        }
    }
}

void ClockPageManager::printFrames(const std::string& result, const std::string& extra_info) const {
    std::cout << "Quadros ";
    for (SIZE_T i = 0; i < num_frames; ++i) {
        std::cout << "[";
        if (frames[i].has_value()) {
            std::cout << *frames[i] << "(R:" << reference_bits[i] << ")";
        }
        else {
            std::cout << "- (R:0)";
        }
        std::cout << "]";
        if (i == clock_hand) {
            std::cout << "<-"; // Indica a posi��o do ponteiro do rel�gio
        }
        std::cout << " ";
    }
    std::cout << "| " << result;
    if (!extra_info.empty()) {
        std::cout << " " << extra_info;
    }
    std::cout << std::endl;
}

void ClockPageManager::printStatistics() const {
    const SIZE_T total_accesses = hits + page_faults;
    const double hit_ratio = (total_accesses > 0) ? (static_cast<double>(hits) / total_accesses) * 100.0 : 0.0;

    std::cout << "\n--- Estat�sticas Finais (Rel�gio) ---\n";
    std::cout << "Total de Acessos:  " << total_accesses << "\n";
    std::cout << "Page Faults (Miss):" << page_faults << "\n";
    std::cout << "Hits:              " << hits << "\n";
    std::cout << "Taxa de Acertos:   " << std::fixed << std::setprecision(2) << hit_ratio << "%\n";
    std::cout << "------------------------------------\n";
}

int main() {
    std::locale::global(std::locale("pt_BR.UTF-8"));

    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simula��o do Rel�gio com " << NUM_FRAMES << " quadros.\n\n";

    ClockPageManager manager(NUM_FRAMES);

    for (const auto& page : reference_string) {
        manager.accessPage(page);
    }

    manager.printStatistics();

    return 0;
}