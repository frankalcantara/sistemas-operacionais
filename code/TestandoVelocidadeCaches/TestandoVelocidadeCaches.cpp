#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <windows.h>

struct CacheInfo {
    int level = 0;
    std::string type;
    long long size_bytes = 0;
};

std::vector<CacheInfo> get_cache_info() {
    std::vector<CacheInfo> caches;

    DWORD buffer_size = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* buffer = nullptr;

    // Primeira chamada para obter o tamanho necessário
    GetLogicalProcessorInformationEx(RelationCache, nullptr, &buffer_size);

    buffer = static_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(malloc(buffer_size));
    if (!buffer) return caches;

    if (!GetLogicalProcessorInformationEx(RelationCache, buffer, &buffer_size)) {
        free(buffer);
        return caches;
    }

    BYTE* ptr = reinterpret_cast<BYTE*>(buffer);
    BYTE* end = ptr + buffer_size;

    while (ptr < end) {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* info =
            reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(ptr);

        if (info->Relationship == RelationCache) {
            CACHE_RELATIONSHIP* cache = &info->Cache;
            CacheInfo cache_info;
            cache_info.level = cache->Level;
            cache_info.size_bytes = cache->CacheSize;

            switch (cache->Type) {
            case CacheUnified:
                cache_info.type = "Unified";
                break;
            case CacheInstruction:
                cache_info.type = "Instruction";
                break;
            case CacheData:
                cache_info.type = "Data";
                break;
            case CacheTrace:
                cache_info.type = "Trace";
                break;
            default:
                cache_info.type = "Unknown";
            }

            caches.push_back(cache_info);
        }

        ptr += info->Size;
    }

    free(buffer);

    // Remove duplicatas mantendo apenas um cache por nível/tipo
    std::vector<CacheInfo> unique_caches;
    for (const auto& cache : caches) {
        bool found = false;
        for (const auto& existing : unique_caches) {
            if (existing.level == cache.level && existing.type == cache.type) {
                found = true;
                break;
            }
        }
        if (!found) {
            unique_caches.push_back(cache);
        }
    }

    return unique_caches;
}

void run_benchmark(long long size_bytes, const std::string& label) {
    if (size_bytes == 0) return;

    // Estrutura de 64 bytes (tamanho típico de linha de cache)
    struct CacheLine {
        size_t next;
        char padding[56]; // Preenche até 64 bytes
    };

    long long num_elements = size_bytes / sizeof(CacheLine);
    if (num_elements < 2) return;

    // Aloca memória alinhada
    std::vector<CacheLine> data(static_cast<size_t>(num_elements));

    // Cria lista encadeada aleatória (pointer chasing)
    std::vector<size_t> indices(static_cast<size_t>(num_elements));
    for (size_t i = 0; i < static_cast<size_t>(num_elements); ++i) {
        indices[i] = i;
    }

    // Embaralha os índices para criar acesso aleatório
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin() + 1, indices.end(), gen);

    // Constrói a lista encadeada
    for (size_t i = 0; i < static_cast<size_t>(num_elements) - 1; ++i) {
        data[indices[i]].next = indices[i + 1];
    }
    data[indices[static_cast<size_t>(num_elements) - 1]].next = indices[0]; // Fecha o ciclo

    // Número de iterações ajustado
    const long long target_accesses = 100000000LL;
    long long iterations = (target_accesses / num_elements);
    if (iterations < 1) iterations = 1;

    // Aquece o cache primeiro
    volatile size_t current = 0;
    for (long long i = 0; i < num_elements; ++i) {
        current = data[current].next;
    }

    // Benchmark real
    auto start = std::chrono::high_resolution_clock::now();

    current = 0;
    for (long long i = 0; i < iterations * num_elements; ++i) {
        current = data[current].next;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    double ns_per_access = (diff.count() * 1e9) / static_cast<double>(iterations * num_elements);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "| " << std::setw(15) << label
        << " | " << std::setw(12) << static_cast<double>(size_bytes) / 1024.0 << " KB"
        << " | " << std::setw(12) << iterations
        << " | " << std::setw(12) << diff.count() << " s"
        << " | " << std::setw(15) << ns_per_access << " ns"
        << " |" << std::endl;

    // Previne otimização
    if (current == static_cast<size_t>(-1)) std::cout << "";
}

int main() {
    // Configurar console para UTF-8 no Windows
    SetConsoleOutputCP(CP_UTF8);

    auto caches = get_cache_info();

    if (caches.empty()) {
        std::cerr << "Não foi possível obter informações de cache. Usando valores padrão." << std::endl;
        caches = {
            {1, "Data", 32 * 1024},
            {2, "Unified", 256 * 1024},
            {3, "Unified", 8 * 1024 * 1024}
        };
    }

    std::cout << "\n============================================" << std::endl;
    std::cout << "   Configuração de Cache Detectada" << std::endl;
    std::cout << "============================================" << std::endl;

    // Ordena os caches por nível
    std::sort(caches.begin(), caches.end(),
        [](const CacheInfo& a, const CacheInfo& b) {
            if (a.level != b.level) return a.level < b.level;
            return a.type < b.type;
        });

    for (const auto& cache : caches) {
        std::cout << "  L" << cache.level << " " << std::setw(12) << cache.type
            << ": " << std::setw(8) << cache.size_bytes / 1024 << " KB" << std::endl;
    }
    std::cout << "============================================\n" << std::endl;

    // Encontra os caches específicos
    CacheInfo* l1_data = nullptr;
    CacheInfo* l2 = nullptr;
    CacheInfo* l3 = nullptr;

    for (auto& cache : caches) {
        if (cache.level == 1 && cache.type == "Data") l1_data = &cache;
        if (cache.level == 2) l2 = &cache;
        if (cache.level == 3) l3 = &cache;
    }

    long long l1_size = l1_data ? l1_data->size_bytes : 32 * 1024;
    long long l2_size = l2 ? l2->size_bytes : 256 * 1024;
    long long l3_size = l3 ? l3->size_bytes : 8 * 1024 * 1024;

    // Define tamanhos de teste com labels descritivos
    struct TestSize {
        long long size;
        std::string label;
    };

    std::vector<TestSize> test_sizes = {
        {8 * 1024, "8 KB"},
        {16 * 1024, "16 KB"},
        {l1_size / 2, "L1/2"},
        {l1_size * 3 / 4, "3L1/4"},
        {l1_size, "L1"},
        {l1_size * 3 / 2, "1.5×L1"},
        {l1_size * 2, "2×L1"},
        {l1_size * 4, "4×L1"},
        {l2_size / 4, "L2/4"},
        {l2_size / 2, "L2/2"},
        {l2_size * 3 / 4, "3L2/4"},
        {l2_size, "L2"},
        {l2_size * 3 / 2, "1.5×L2"},
        {l2_size * 2, "2×L2"},
        {l3_size / 4, "L3/4"},
        {l3_size / 2, "L3/2"},
        {l3_size * 3 / 4, "3L3/4"},
        {l3_size, "L3"},
        {l3_size * 2, "2×L3"},
        {l3_size * 4, "4×L3"},
        {32 * 1024 * 1024, "32 MB"},
        {64 * 1024 * 1024, "64 MB"}
    };

    // Remove duplicatas e ordena por tamanho
    std::sort(test_sizes.begin(), test_sizes.end(),
        [](const TestSize& a, const TestSize& b) { return a.size < b.size; });

    auto last = std::unique(test_sizes.begin(), test_sizes.end(),
        [](const TestSize& a, const TestSize& b) { return a.size == b.size; });
    test_sizes.erase(last, test_sizes.end());

    std::cout << "Benchmark de Latência de Memória (Pointer Chasing)" << std::endl;
    std::cout << "Método: Lista encadeada com acesso aleatório\n" << std::endl;
    std::cout << "+-----------------+-----------------+--------------+----------------+--------------------+" << std::endl;
    std::cout << "|      Região     |   Tamanho       |  Repetições  | Tempo Total    | Latência/Acesso    |" << std::endl;
    std::cout << "+-----------------+-----------------+--------------+----------------+--------------------+" << std::endl;

    for (const auto& test : test_sizes) {
        if (test.size > 0 && test.size <= 128 * 1024 * 1024) { // Limita a 128MB
            run_benchmark(test.size, test.label);
        }
    }

    std::cout << "+-----------------+-----------------+--------------+----------------+--------------------+" << std::endl;

    std::cout << "\nPressione Enter para sair...";
    std::cin.get();

    return 0;
}