/**
 * @file main_lru.cpp
 * @author Frank Alcantara 
 * @brief Implementação de uma simulação do algoritmo de substituição de páginas LRU (Least Recently Used).
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este código simula o algoritmo LRU, que se aproxima do Ótimo baseando-se no
 * princípio da localidade temporal. A implementação utiliza uma lista duplamente ligada
 * e uma tabela de hash para garantir operações de alta performance (O(1)). O código é
 * escrito em C++23 e usa tipos de dados do Windows SDK.
 */

#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <optional>
#include <string>
#include <format>

 // Inclui o SDK do Windows para tipos de dados nativos como DWORD e SIZE_T
#include <Windows.h>

/**
 * @class LruPageManager
 * @brief Gerencia a substituição de páginas de memória usando o algoritmo LRU.
 *
 * @details Esta classe encapsula a lógica de simulação do algoritmo LRU. Ela mantém
 * uma lista com a ordem de recenticidade de uso das páginas e uma tabela de hash
 * para acesso rápido, garantindo performance de O(1) nas operações.
 */
class LruPageManager {
public:
    /**
     * @brief Construtor da classe LruPageManager.
     * @param total_frames O número total de quadros de memória física disponíveis.
     */
    explicit LruPageManager(SIZE_T total_frames);

    /**
     * @brief Simula o acesso a uma página de memória.
     *
     * @details Verifica se a página está na memória. Se sim (hit), a move para a
     * frente da lista de recenticidade. Se não (miss), a insere na frente,
     * removendo a página menos recentemente usada se a memória estiver cheia.
     *
     * @param page_number O número da página a ser acessada.
     */
    void accessPage(DWORD page_number);

    /**
     * @brief Imprime as estatísticas finais da simulação.
     */
    void printStatistics() const;

private:
    /**
     * @brief Imprime o estado atual dos quadros de memória na ordem de recenticidade.
     * @param result Uma string descrevendo o resultado do último acesso (HIT ou MISS).
     * @param victim_info Uma string opcional com informação sobre a página vítima.
     */
    void printFrames(const std::string& result, const std::string& victim_info = "") const;

    /// @brief O número total de quadros de memória física.
    const SIZE_T num_frames;

    /// @brief Lista duplamente ligada que mantém a ordem de uso. Frente = MRU, Fundo = LRU.
    std::list<DWORD> lru_list;

    /// @brief Tabela de páginas para busca rápida. Mapeia um número de página para seu iterador na lru_list.
    std::unordered_map<DWORD, std::list<DWORD>::iterator> page_table;

    /// @brief Contador de falhas de página (page faults/misses).
    SIZE_T page_faults;

    /// @brief Contador de acertos de página (hits).
    SIZE_T hits;
};

LruPageManager::LruPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    page_faults(0),
    hits(0) {
}

void LruPageManager::accessPage(DWORD page_number) {
    std::cout << std::format("Página {:>2} | ", page_number);

    // 1. Verificar se a página está na memória (HIT)
    auto it = page_table.find(page_number);
    if (it != page_table.end()) {
        hits++;
        // Em um HIT, a página se torna a mais recentemente usada.
        // Movemos o elemento correspondente para o início da lista.
        // A operação splice é O(1).
        lru_list.splice(lru_list.begin(), lru_list, it->second);
        printFrames("HIT", std::format("({} move para frente)", page_number));
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Se a memória está cheia, remover a página menos recentemente usada (LRU)
    if (page_table.size() == num_frames) {
        // A página LRU é a última na lista
        DWORD victim_page = lru_list.back();

        // Remove a vítima da tabela de hash e da lista
        page_table.erase(victim_page);
        lru_list.pop_back();

        // Insere a nova página na frente (agora é a MRU)
        lru_list.push_front(page_number);
        page_table[page_number] = lru_list.begin();

        printFrames("MISS", std::format("(remove {})", victim_page));
    }
    else {
        // 4. Se há quadro livre, apenas insere a nova página na frente
        lru_list.push_front(page_number);
        page_table[page_number] = lru_list.begin();
        printFrames("MISS");
    }
}

void LruPageManager::printFrames(const std::string& result, const std::string& victim_info) const {
    std::cout << "Quadros (ordem LRU) [";
    for (auto it = lru_list.begin(); it != lru_list.end(); ++it) {
        std::cout << *it;
        if (std::next(it) != lru_list.end()) {
            std::cout << ", ";
        }
    }
    std::cout << std::format("] | {} {}", result, victim_info) << std::endl;
}


void LruPageManager::printStatistics() const {
    const SIZE_T total_accesses = hits + page_faults;
    const double hit_ratio = (total_accesses > 0) ? (static_cast<double>(hits) / total_accesses) * 100.0 : 0.0;

    std::cout << "\n--- Estatísticas Finais (LRU) ---\n";
    std::cout << std::format("Total de Acessos:  {}\n", total_accesses);
    std::cout << std::format("Page Faults (Miss):{}\n", page_faults);
    std::cout << std::format("Hits:              {}\n", hits);
    std::cout << std::format("Taxa de Acertos:   {:.2f}%\n", hit_ratio);
    std::cout << "--------------------------------\n";
}

/**
 * @brief Função principal que executa a simulação do algoritmo LRU.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    std::locale::global(std::locale("pt_BR.UTF-8"));

    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simulação LRU com " << NUM_FRAMES << " quadros.\n\n";

    LruPageManager manager(NUM_FRAMES);

    for (const auto& page : reference_string) {
        manager.accessPage(page);
    }

    manager.printStatistics();

    return 0;
}