/**
 * @file tlb_performance_simulator.cpp
 * @brief Simulador de performance da Translation Lookaside Buffer (TLB)
 * @author Livro de Sistemas Operacionais
 * @version 1.0
 * @date 2025
 *
 * Este programa demonstra como calcular o tempo efetivo de acesso à memória
 * considerando a performance da TLB usando a fórmula:
 *
 * Effective Access Time = TLB Hit Time + TLB Miss Rate × Page Table Access Time
 *
 * O programa simula diferentes cenários de carga de trabalho e analisa
 * o impacto da TLB na performance geral do sistema.
 */

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <format>
#include <ranges>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <memory>

 /**
  * @brief Estrutura para armazenar métricas de performance da TLB
  */
struct TLBMetrics {
    double tlb_hit_time;           ///< Tempo de acesso em caso de TLB hit (ciclos)
    double page_table_access_time; ///< Tempo de acesso à tabela de páginas (ciclos)
    size_t total_accesses;         ///< Total de acessos à memória
    size_t tlb_hits;              ///< Número de TLB hits
    size_t tlb_misses;            ///< Número de TLB misses

    /**
     * @brief Calcula a taxa de TLB miss
     * @return Taxa de miss como valor entre 0.0 e 1.0
     */
    double getMissRate() const {
        return total_accesses > 0 ? static_cast<double>(tlb_misses) / total_accesses : 0.0;
    }

    /**
     * @brief Calcula a taxa de TLB hit
     * @return Taxa de hit como valor entre 0.0 e 1.0
     */
    double getHitRate() const {
        return total_accesses > 0 ? static_cast<double>(tlb_hits) / total_accesses : 0.0;
    }

    /**
     * @brief Calcula o tempo efetivo de acesso usando a fórmula fundamental
     * @return Tempo efetivo de acesso em ciclos de clock
     *
     * Implementa a fórmula:
     * Effective Access Time = TLB Hit Time + TLB Miss Rate × Page Table Access Time
     */
    double getEffectiveAccessTime() const {
        return tlb_hit_time + (getMissRate() * page_table_access_time);
    }
};

/**
 * @brief Enumeração dos tipos de padrão de acesso à memória
 */
enum class AccessPattern {
    SEQUENTIAL,    ///< Acesso sequencial (boa localidade)
    RANDOM,        ///< Acesso aleatório (localidade pobre)
    STRIDE,        ///< Acesso com stride fixo
    MIXED          ///< Padrão misto de acesso
};

/**
 * @brief Classe que simula o comportamento de uma TLB
 */
class TLBSimulator {
private:
    size_t tlb_size_;                              ///< Tamanho da TLB em entradas
    std::unordered_map<uint64_t, bool> tlb_cache_; ///< Cache simulado da TLB
    TLBMetrics metrics_;                           ///< Métricas coletadas
    std::mt19937 generator_;                       ///< Gerador de números aleatórios

    /**
     * @brief Remove entrada mais antiga da TLB (LRU simples)
     */
    void evictOldestEntry() {
        if (tlb_cache_.size() >= tlb_size_) {
            // Simula política LRU removendo primeira entrada
            auto it = tlb_cache_.begin();
            tlb_cache_.erase(it);
        }
    }

public:
    /**
     * @brief Construtor do simulador TLB
     * @param tlb_size Tamanho da TLB em entradas
     * @param tlb_hit_time Tempo de acesso em caso de hit (ciclos)
     * @param page_table_time Tempo de acesso à tabela de páginas (ciclos)
     */
    TLBSimulator(size_t tlb_size, double tlb_hit_time, double page_table_time)
        : tlb_size_(tlb_size), generator_(std::random_device{}()) {
        metrics_.tlb_hit_time = tlb_hit_time;
        metrics_.page_table_access_time = page_table_time;
        metrics_.total_accesses = 0;
        metrics_.tlb_hits = 0;
        metrics_.tlb_misses = 0;
    }

    /**
     * @brief Simula um acesso à memória virtual
     * @param virtual_address Endereço virtual sendo acessado
     * @return true se foi TLB hit, false se foi TLB miss
     */
    bool accessMemory(uint64_t virtual_address) {
        // Extrai número da página virtual (assumindo páginas de 4KB)
        uint64_t page_number = virtual_address >> 12;

        metrics_.total_accesses++;

        // Verifica se a página está na TLB
        if (tlb_cache_.contains(page_number)) {
            metrics_.tlb_hits++;
            return true; // TLB hit
        }
        else {
            metrics_.tlb_misses++;

            // TLB miss: precisa acessar tabela de páginas
            // Adiciona entrada na TLB
            evictOldestEntry();
            tlb_cache_[page_number] = true;

            return false; // TLB miss
        }
    }

    /**
     * @brief Gera padrão de acesso sequencial
     * @param start_address Endereço inicial
     * @param num_accesses Número de acessos
     * @param stride Tamanho do stride em bytes
     * @return Vetor de endereços virtuais
     */
    std::vector<uint64_t> generateSequentialPattern(uint64_t start_address,
        size_t num_accesses,
        size_t stride = 4096) {
        std::vector<uint64_t> addresses;
        addresses.reserve(num_accesses);

        for (size_t i = 0; i < num_accesses; ++i) {
            addresses.push_back(start_address + (i * stride));
        }

        return addresses;
    }

    /**
     * @brief Gera padrão de acesso aleatório
     * @param num_accesses Número de acessos
     * @param address_range Faixa de endereços
     * @return Vetor de endereços virtuais aleatórios
     */
    std::vector<uint64_t> generateRandomPattern(size_t num_accesses,
        uint64_t address_range = 0x100000000ULL) {
        std::vector<uint64_t> addresses;
        addresses.reserve(num_accesses);

        std::uniform_int_distribution<uint64_t> dist(0, address_range);

        for (size_t i = 0; i < num_accesses; ++i) {
            // Alinha endereços em limites de página
            uint64_t addr = dist(generator_) & ~0xFFFULL;
            addresses.push_back(addr);
        }

        return addresses;
    }

    /**
     * @brief Executa simulação com padrão específico de acesso
     * @param addresses Vetor de endereços para acessar
     */
    void runSimulation(const std::vector<uint64_t>& addresses) {
        for (uint64_t addr : addresses) {
            accessMemory(addr);
        }
    }

    /**
     * @brief Reseta estatísticas do simulador
     */
    void reset() {
        tlb_cache_.clear();
        metrics_.total_accesses = 0;
        metrics_.tlb_hits = 0;
        metrics_.tlb_misses = 0;
    }

    /**
     * @brief Obtém métricas atuais
     * @return Estrutura com métricas de performance
     */
    const TLBMetrics& getMetrics() const {
        return metrics_;
    }
};

/**
 * @brief Classe para análise comparativa de diferentes configurações
 */
class TLBAnalyzer {
private:
    std::vector<std::unique_ptr<TLBSimulator>> simulators_; ///< Simuladores para comparar

public:
    /**
     * @brief Adiciona simulador à análise
     * @param simulator Simulador único para análise
     */
    void addSimulator(std::unique_ptr<TLBSimulator> simulator) {
        simulators_.push_back(std::move(simulator));
    }

    /**
     * @brief Executa análise comparativa
     * @param pattern Padrão de acesso à memória
     * @param num_accesses Número de acessos para simular
     */
    void runComparison(AccessPattern pattern, size_t num_accesses = 10000) {
        std::cout << std::format("\n=== Análise Comparativa de TLB ===\n");
        std::cout << std::format("Padrão de acesso: {}\n",
            pattern == AccessPattern::SEQUENTIAL ? "Sequencial" :
            pattern == AccessPattern::RANDOM ? "Aleatório" :
            pattern == AccessPattern::STRIDE ? "Stride" : "Misto");
        std::cout << std::format("Número de acessos: {}\n\n", num_accesses);

        for (size_t i = 0; i < simulators_.size(); ++i) {
            auto& sim = simulators_[i];
            sim->reset();

            // Gera padrão de acesso apropriado
            std::vector<uint64_t> addresses;
            switch (pattern) {
            case AccessPattern::SEQUENTIAL:
                addresses = sim->generateSequentialPattern(0x10000000, num_accesses);
                break;
            case AccessPattern::RANDOM:
                addresses = sim->generateRandomPattern(num_accesses);
                break;
            case AccessPattern::STRIDE:
                addresses = sim->generateSequentialPattern(0x10000000, num_accesses, 8192);
                break;
            case AccessPattern::MIXED:
                // Metade sequencial, metade aleatória
                auto seq = sim->generateSequentialPattern(0x10000000, num_accesses / 2);
                auto rand = sim->generateRandomPattern(num_accesses / 2);
                addresses.insert(addresses.end(), seq.begin(), seq.end());
                addresses.insert(addresses.end(), rand.begin(), rand.end());
                break;
            }

            // Executa simulação
            sim->runSimulation(addresses);

            // Coleta e exibe resultados
            const auto& metrics = sim->getMetrics();

            std::cout << std::format("--- Simulador {} ---\n", i + 1);
            std::cout << std::format("Total de acessos: {}\n", metrics.total_accesses);
            std::cout << std::format("TLB Hits: {} ({:.2f}%)\n",
                metrics.tlb_hits, metrics.getHitRate() * 100);
            std::cout << std::format("TLB Misses: {} ({:.2f}%)\n",
                metrics.tlb_misses, metrics.getMissRate() * 100);
            std::cout << std::format("Tempo TLB Hit: {:.1f} ciclos\n", metrics.tlb_hit_time);
            std::cout << std::format("Tempo Page Table: {:.1f} ciclos\n", metrics.page_table_access_time);
            std::cout << std::format("🔹 Tempo Efetivo de Acesso: {:.2f} ciclos\n",
                metrics.getEffectiveAccessTime());

            // Calcula impacto da performance
            double ideal_time = metrics.tlb_hit_time;
            double overhead = (metrics.getEffectiveAccessTime() - ideal_time) / ideal_time * 100;
            std::cout << std::format("Custo Computacional Extra devido a TLB misses: {:.2f}%\n\n", overhead);
        }
    }
};

/**
 * @brief Demonstra diferentes cenários de working set
 */
void demonstrateWorkingSetImpact() {
    std::cout << "\n=== Demonstração: Impacto do Working Set ===\n\n";

    // TLB pequena com 64 entradas
    TLBSimulator small_tlb(64, 1.0, 100.0);

    std::vector<size_t> working_set_sizes = { 32, 64, 128, 256, 512 };

    for (size_t ws_size : working_set_sizes) {
        small_tlb.reset();

        // Gera working set: acessa as mesmas páginas repetidamente
        std::vector<uint64_t> addresses;
        for (int iteration = 0; iteration < 100; ++iteration) {
            for (size_t page = 0; page < ws_size; ++page) {
                addresses.push_back(page * 4096); // Páginas de 4KB
            }
        }

        small_tlb.runSimulation(addresses);
        const auto& metrics = small_tlb.getMetrics();

        std::cout << std::format("Working Set: {} páginas | TLB Hit Rate: {:.2f}% | "
            "Tempo Efetivo: {:.2f} ciclos\n",
            ws_size, metrics.getHitRate() * 100,
            metrics.getEffectiveAccessTime());
    }
}

/**
 * @brief Função principal demonstrando o uso do simulador
 */
int main() {
    std::cout << "=== Simulador de Performance TLB - C++ 23 ===\n";
    std::cout << "Implementação da fórmula: Effective Access Time = TLB Hit Time + TLB Miss Rate × Page Table Access Time\n";

    // Cria analisador
    TLBAnalyzer analyzer;

    // Adiciona diferentes configurações de TLB
    analyzer.addSimulator(std::make_unique<TLBSimulator>(64, 1.0, 100.0));   // TLB pequena
    analyzer.addSimulator(std::make_unique<TLBSimulator>(256, 2.0, 100.0));  // TLB média
    analyzer.addSimulator(std::make_unique<TLBSimulator>(1024, 5.0, 100.0)); // TLB grande

    // Testa diferentes padrões
    analyzer.runComparison(AccessPattern::SEQUENTIAL, 5000);
    analyzer.runComparison(AccessPattern::RANDOM, 5000);
    analyzer.runComparison(AccessPattern::MIXED, 5000);

    // Demonstra impacto do working set
    demonstrateWorkingSetImpact();

    // Análise matemática da fórmula
    std::cout << "\n=== Análise Matemática da Fórmula ===\n\n";

    double tlb_hit_time = 2.0;
    double page_table_time = 100.0;

    std::vector<double> miss_rates = { 0.01, 0.05, 0.10, 0.20, 0.50 };

    std::cout << "Miss Rate | Effective Time | Overhead\n";
    std::cout << "----------|----------------|----------\n";

    for (double miss_rate : miss_rates) {
        double effective_time = tlb_hit_time + (miss_rate * page_table_time);
        double overhead = ((effective_time - tlb_hit_time) / tlb_hit_time) * 100;

        std::cout << std::format("{:8.1f}% | {:13.2f} | {:7.1f}%\n",
            miss_rate * 100, effective_time, overhead);
    }

    std::cout << "\n Conclusões:\n";
    std::cout << "TLB miss rate tem impacto dramático na performance\n";
    std::cout << "• Working sets pequenos maximizam TLB hit rate\n";
    std::cout << "• TLBs maiores reduzem miss rate mas aumentam hit time\n";
    std::cout << "• Localidade de acesso é fundamental para eficiência\n";

    return 0;
}