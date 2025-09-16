/**
 * @file main_lru.cpp
 * @author Frank Alcantara 
 * @brief Implementa��o de uma simula��o do algoritmo de substitui��o de p�ginas LRU (Least Recently Used).
 * @version 1.0
 * @date 2025-09-14
 *
 * @copyright Copyright (c) 2025
 *
 * @details Este c�digo simula o algoritmo LRU, que se aproxima do �timo baseando-se no
 * princ�pio da localidade temporal. A implementa��o utiliza uma lista duplamente ligada
 * e uma tabela de hash para garantir opera��es de alta performance (O(1)). O c�digo �
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
 * @brief Gerencia a substitui��o de p�ginas de mem�ria usando o algoritmo LRU.
 *
 * @details Esta classe encapsula a l�gica de simula��o do algoritmo LRU. Ela mant�m
 * uma lista com a ordem de recenticidade de uso das p�ginas e uma tabela de hash
 * para acesso r�pido, garantindo performance de O(1) nas opera��es.
 */
class LruPageManager {
public:
    /**
     * @brief Construtor da classe LruPageManager.
     * @param total_frames O n�mero total de quadros de mem�ria f�sica dispon�veis.
     */
    explicit LruPageManager(SIZE_T total_frames);

    /**
     * @brief Simula o acesso a uma p�gina de mem�ria.
     *
     * @details Verifica se a p�gina est� na mem�ria. Se sim (hit), a move para a
     * frente da lista de recenticidade. Se n�o (miss), a insere na frente,
     * removendo a p�gina menos recentemente usada se a mem�ria estiver cheia.
     *
     * @param page_number O n�mero da p�gina a ser acessada.
     */
    void accessPage(DWORD page_number);

    /**
     * @brief Imprime as estat�sticas finais da simula��o.
     */
    void printStatistics() const;

private:
    /**
     * @brief Imprime o estado atual dos quadros de mem�ria na ordem de recenticidade.
     * @param result Uma string descrevendo o resultado do �ltimo acesso (HIT ou MISS).
     * @param victim_info Uma string opcional com informa��o sobre a p�gina v�tima.
     */
    void printFrames(const std::string& result, const std::string& victim_info = "") const;

    /// @brief O n�mero total de quadros de mem�ria f�sica.
    const SIZE_T num_frames;

    /// @brief Lista duplamente ligada que mant�m a ordem de uso. Frente = MRU, Fundo = LRU.
    std::list<DWORD> lru_list;

    /// @brief Tabela de p�ginas para busca r�pida. Mapeia um n�mero de p�gina para seu iterador na lru_list.
    std::unordered_map<DWORD, std::list<DWORD>::iterator> page_table;

    /// @brief Contador de falhas de p�gina (page faults/misses).
    SIZE_T page_faults;

    /// @brief Contador de acertos de p�gina (hits).
    SIZE_T hits;
};

LruPageManager::LruPageManager(SIZE_T total_frames)
    : num_frames(total_frames),
    page_faults(0),
    hits(0) {
}

void LruPageManager::accessPage(DWORD page_number) {
    std::cout << std::format("P�gina {:>2} | ", page_number);

    // 1. Verificar se a p�gina est� na mem�ria (HIT)
    auto it = page_table.find(page_number);
    if (it != page_table.end()) {
        hits++;
        // Em um HIT, a p�gina se torna a mais recentemente usada.
        // Movemos o elemento correspondente para o in�cio da lista.
        // A opera��o splice � O(1).
        lru_list.splice(lru_list.begin(), lru_list, it->second);
        printFrames("HIT", std::format("({} move para frente)", page_number));
        return;
    }

    // 2. Se ausente (MISS / Page Fault)
    page_faults++;

    // 3. Se a mem�ria est� cheia, remover a p�gina menos recentemente usada (LRU)
    if (page_table.size() == num_frames) {
        // A p�gina LRU � a �ltima na lista
        DWORD victim_page = lru_list.back();

        // Remove a v�tima da tabela de hash e da lista
        page_table.erase(victim_page);
        lru_list.pop_back();

        // Insere a nova p�gina na frente (agora � a MRU)
        lru_list.push_front(page_number);
        page_table[page_number] = lru_list.begin();

        printFrames("MISS", std::format("(remove {})", victim_page));
    }
    else {
        // 4. Se h� quadro livre, apenas insere a nova p�gina na frente
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

    std::cout << "\n--- Estat�sticas Finais (LRU) ---\n";
    std::cout << std::format("Total de Acessos:  {}\n", total_accesses);
    std::cout << std::format("Page Faults (Miss):{}\n", page_faults);
    std::cout << std::format("Hits:              {}\n", hits);
    std::cout << std::format("Taxa de Acertos:   {:.2f}%\n", hit_ratio);
    std::cout << "--------------------------------\n";
}

/**
 * @brief Fun��o principal que executa a simula��o do algoritmo LRU.
 * @return int Retorna 0 em caso de sucesso.
 */
int main() {
    std::locale::global(std::locale("pt_BR.UTF-8"));

    const SIZE_T NUM_FRAMES = 3;
    const std::vector<DWORD> reference_string = {
        7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1
    };

    std::cout << "Iniciando Simula��o LRU com " << NUM_FRAMES << " quadros.\n\n";

    LruPageManager manager(NUM_FRAMES);

    for (const auto& page : reference_string) {
        manager.accessPage(page);
    }

    manager.printStatistics();

    return 0;
}