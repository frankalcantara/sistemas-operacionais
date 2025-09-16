/**
 * @file main_aging.cpp
 * @author Gemini
 * @brief Implementação compatível do algoritmo de substituição de páginas Aging (Envelhecimento).
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este código simula o algoritmo Aging, que aproxima o LRU usando contadores
 * de múltiplos bits para rastrear o histórico de acesso. Um timer simulado dispara
 * periodicamente para "envelhecer" as páginas. A versão usa iostream/iomanip para
 * máxima compatibilidade.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>
#include <iomanip>
#include <cstdint> // Para uint8_t
#include <bitset>  // Para exibir contadores em binário
#include <limits>  // Para std::numeric_limits

 // Inclui o SDK do Windows para tipos de dados nativos como DWORD e SIZE_T
#include <Windows.h>

// Constante para o número de bits do contador de envelhecimento
const int AGING_COUNTER_BITS = 8;

/**
 * @class AgingPageManager
 * @brief Gerencia a substituição de páginas usando o algoritmo de Aging.
 *
 * @details A classe simula quadros de memória, cada um associado a um bit de referência
 * e um contador de envelhecimento. A substituição é baseada no menor valor do contador.
 */
class AgingPageManager {
public:
    explicit AgingPageManager(SIZE_T total_frames);
    void accessPage(DWORD page_number);
    void timerTick();
    void printStatistics() const;

private:
    void printFrames(const std::string& event, const std::string& details = "") const;

    const SIZE_T num_frames;
    std::vector<std::optional<DWORD>> frames;
    std::vector<bool> reference_bits;
    std::vector<uint8_t> age_counters; // Contador de 8 bits
    std::unordered_map<DWORD, SIZE_T> page_table;
    SIZE_T page_faults;
    SIZE_T hits;
};

AgingPageManager::AgingPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    page_faults(0),
    hits(0) {
    frames.resize(num_frames, std::nullopt);
    reference_bits.resize(num_frames, false);
    age_counters.resize(num_frames, 0);
}

void AgingPageManager::accessPage(DWORD page_number) {
    auto it = page_table.find(page_number);

    // 1. Verificar se a página está na memória (HIT)
    if (it != page_table.end()) {
        hits++;
        reference_bits[it->second] = true; // Marca como referenciada
        printFrames("Acesso PG " + std::to_string(page_number), "HIT");
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    std::optional<SIZE_T> free_frame;
    for (SIZE_T i = 0; i < num_frames; ++i) {
        if (!frames[i].has_value()) {
            free_frame = i;
            break;
        }
    }

    // 3. Se há quadro livre
    if (free_frame.has_value()) {
        SIZE_T frame_index = *free_frame;
        frames[frame_index] = page_number;
        reference_bits[frame_index] = true;
        age_counters[frame_index] = 0; // Reseta o contador
        page_table[page_number] = frame_index;
        printFrames("Acesso PG " + std::to_string(page_number), "MISS (quadro livre)");
    }
    else {
        // 4. Se a memória está cheia, encontrar a vítima com o menor contador
        uint8_t min_age = std::numeric_limits<uint8_t>::max();
        SIZE_T victim_frame_index = 0;

        for (SIZE_T i = 0; i < num_frames; ++i) {
            if (age_counters[i] < min_age) {
                min_age = age_counters[i];
                victim_frame_index = i;
            }
        }

        const DWORD victim_page = frames[victim_frame_index].value();

        // Remove a vítima e insere a nova página
        page_table.erase(victim_page);
        frames[victim_frame_index] = page_number;
        reference_bits[victim_frame_index] = true;
        age_counters[victim_frame_index] = 0;
        page_table[page_number] = victim_frame_index;

        std::string details = "MISS (remove PG " + std::to_string(victim_page) + ")";
        printFrames("Acesso PG " + std::to_string(page_number), details);
    }
}

void AgingPageManager::timerTick() {
    printFrames("TIMER TICK", "Atualizando contadores...");
    const uint8_t msb_mask = (1 << (AGING_COUNTER_BITS - 1));

    for (SIZE_T i = 0; i < num_frames; ++i) {
        if (frames[i].has_value()) {
            // a. Deslocar contador 1 bit à direita
            age_counters[i] >>= 1;

            // b. Se bit_referência = 1: definir bit mais significativo = 1
            if (reference_bits[i]) {
                age_counters[i] |= msb_mask;
            }

            // c. Limpar bit_referência
            reference_bits[i] = false;
        }
    }
    printFrames("POST-TICK", "Contadores atualizados");
}


void AgingPageManager::printFrames(const std::string& event, const std::string& details) const {
    std::cout << "--- Evento: " << std::left << std::setw(15) << event << " | " << details << " ---\n";
    for (SIZE_T i = 0; i < num_frames; ++i) {
        std::cout << "Quadro " << i << ": ";
        if (frames[i].has_value()) {
            std::cout << "PG " << std::setw(2) << *frames[i] << " | "
                << "R: " << reference_bits[i] << " | "
                << "Age: " << std::bitset<AGING_COUNTER_BITS>(age_counters[i]);
        }
        else {
            std::cout << "Vazio";
        }
        std::cout << "\n";
    }
    std::cout << "--------------------------------------------------------\n";
}


void AgingPageManager::printStatistics() const {
    const SIZE_T total_accesses = hits + page_faults;
    const double hit_ratio = (total_accesses > 0) ? (static_cast<double>(hits) / total_accesses) * 100.0 : 0.0;

    std::cout << "\n--- Estatísticas Finais (Aging) ---\n";
    std::cout << "Total de Acessos:  " << total_accesses << "\n";
    std::cout << "Page Faults (Miss):" << page_faults << "\n";
    std::cout << "Hits:              " << hits << "\n";
    std::cout << "Taxa de Acertos:   " << std::fixed << std::setprecision(2) << hit_ratio << "%\n";
    std::cout << "----------------------------------\n";
}

int main() {
    std::locale::global(std::locale("pt_BR.UTF-8"));

    const SIZE_T NUM_FRAMES = 3;
    const int TICK_INTERVAL = 4; // O timer "dispara" a cada 4 acessos a páginas
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simulação de Aging com " << NUM_FRAMES << " quadros e tick a cada " << TICK_INTERVAL << " acessos.\n\n";

    AgingPageManager manager(NUM_FRAMES);

    for (size_t i = 0; i < reference_string.size(); ++i) {
        manager.accessPage(reference_string[i]);

        // Simula a interrupção do timer
        if ((i + 1) % TICK_INTERVAL == 0) {
            manager.timerTick();
        }
    }

    manager.printStatistics();

    return 0;
}