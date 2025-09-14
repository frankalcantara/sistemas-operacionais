#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <windows.h>
#include <psapi.h>

/**
 * @brief Obtém o uso de memória do processo atual usando o Windows SDK.
 * @return O tamanho do Working Set em bytes, ou 0 em caso de erro.
 *
 * @details Usa GetProcessMemoryInfo para obter o Working Set Size,
 * que representa a memória física usada pelo processo.
 */
size_t get_process_memory_usage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
    return 0; // Retorna 0 em caso de erro
}

/**
 * @brief Estima o uso de memória de um std::vector usando a STL.
 * @param vec O vetor cuja memória será estimada.
 * @return O total estimado de memória em bytes.
 *
 * @details Calcula a memória alocada pelo vetor (capacity * sizeof(T))
 * e adiciona o overhead do std::vector (3 ponteiros em sistemas 64-bit).
 */
size_t estimate_vector_memory(const std::vector<char>& vec) {
    size_t elements_memory = vec.capacity() * sizeof(char);
    size_t vector_overhead = 3 * sizeof(void*); // 3 ponteiros em sistemas 64-bit
    size_t allocator_overhead = vec.capacity() > 0 ? 16 : 0; // Overhead estimado do alocador
    return elements_memory + vector_overhead + allocator_overhead;
}

/**
 * @brief Mede e imprime o tempo de execução e uso de memória de uma tarefa.
 * @param task_name Nome da tarefa.
 * @param task Função a ser executada.
 * @param vec Vetor para estimativa de memória (se aplicável).
 *
 * @details Mede o tempo com std::chrono e o uso de memória com GetProcessMemoryInfo.
 * Se o vetor for fornecido, também calcula sua memória estimada via STL.
 */
void measure_execution(const std::string& task_name, const std::function<void()>& task,
    const std::vector<char>& vec = std::vector<char>()) {
    std::cout << "--- Executando Tarefa: " << task_name << " ---\n";

    // Medir memória antes
    size_t memory_before = get_process_memory_usage();

    // Medir tempo
    auto start = std::chrono::high_resolution_clock::now();
    task();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    // Medir memória depois
    size_t memory_after = get_process_memory_usage();

    // Imprimir resultados
    std::cout << "[Tempo de execução]: " << duration.count() << " nanosegundos.\n";
    if (memory_before > 0 && memory_after > 0) {
        std::cout << "[Memória do processo]: Antes = " << memory_before / 1024.0
            << " KB, Depois = " << memory_after / 1024.0
            << " KB, Diferença = " << (memory_after - memory_before) / 1024.0 << " KB.\n";
    }
    else {
        std::cout << "[Memória do processo]: Erro ao obter informações de memória.\n";
    }
    if (!vec.empty()) {
        std::cout << "[Memória do vetor (estimada)]: " << estimate_vector_memory(vec)
            << " bytes (vetor + overhead).\n";
    }
}

/**
 * @brief Cria um std::vector com um número específico de caracteres aleatórios.
 */
std::vector<char> create_random_array(const size_t size) {
    std::vector<char> arr;
    arr.reserve(size);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(33, 126);
    for (size_t i = 0; i < size; ++i) {
        arr.push_back(static_cast<char>(distrib(gen)));
    }
    return arr;
}

/**
  @brief Ordena um array de caracteres.
 * @param arr O vetor de caracteres a ser ordenado. A ordenacao e feita in-place.
 *
 * @details Utiliza a funcao std::sort do cabecalho <algorithm>, que e altamente
 * otimizada (geralmente uma implementacao de Introsort).
 */
void sort_array(std::vector<char>& arr) {
    std::sort(arr.begin(), arr.end());
}

/**
  @brief Ordena um array de caracteres.
 * @param arr O vetor de caracteres a ser ordenado. A ordenacao e feita in-place.
 *
 * @details Utiliza a funcao std::sort do cabecalho <algorithm>, que e altamente
 * otimizada (geralmente uma implementacao de Introsort).
 */
void print_array(const std::vector<char>& arr) {
	// imprimir caracter por caracter é lento e ineficiente
	// melhor construir uma string e imprimir de uma vez
    std::string output;
    output.reserve(2 * arr.size()); // Espaço para caracteres + espaços
    for (const char c : arr) {
        output += c;
        output += ' ';
    }
    std::cout << output << '\n';
}

/**
  @brief Calcula uma aproximacao de PI usando a formula de Leibniz.
 * @param iterations O numero de iteracoes a serem usadas na serie.
 * @return Uma aproximacao de PI como um long double.
 *
 * @details A formula e: PI = 4 * sum_{k=0 to inf} ((-1)^k / (2k + 1)).
 * @warning A serie de Leibniz converge muito lentamente. Um numero extremamente
 * grande de iteracoes e necessario para uma boa precisao, e a precisao maxima
 * e limitada pelo tipo long double. Calcular 100 casas decimais nao e
 * viavel com esta abordagem e tipos de dados padrao.
 *
 * Além disso, existirão imprecisões devido à natureza da aritmética de ponto flutuante.
 *
 *
 */
long double calculate_pi_leibniz(const long long iterations) {
    long double pi_approx = 0.0L;
    for (long long k = 0; k < iterations; ++k) {
        if (k % 2 == 0) { // Termo par (k=0, 2, 4...)
            pi_approx += 4.0L / (2.0L * k + 1.0L);
        }
        else { // Termo impar (k=1, 3, 5...)
            pi_approx -= 4.0L / (2.0L * k + 1.0L);
        }
    }
    return pi_approx;
}

/**
 * @brief Função principal com medição de memória melhorada.
 */
int main() {
	// apenas para os acentos do português no terminal do Windows
	// este código está em UTF-8
    SetConsoleOutputCP(CP_UTF8); // Define a saída do console para UTF-8
    SetConsoleCP(CP_UTF8);

    const size_t ARRAY_SIZE = 1000;
    std::vector<char> my_array;

    // --- Tarefa (a): Criar o array ---
    measure_execution("a) Criar um array com 1000 caracteres aleatórios", [&]() {
        my_array = create_random_array(ARRAY_SIZE);
        }, my_array);

    // --- Tarefa (b): Ordenar o array ---
    measure_execution("b) Ordenar o array com 1000 caracteres", [&]() {
        sort_array(my_array);
        }, my_array);

    // --- Tarefa (c): Imprimir o array ---
    measure_execution("c) Imprimir o array com 1000 caracteres no terminal", [&]() {
        print_array(my_array);
        }, my_array);

    // --- Tarefa (d): Calcular PI ---
    long double pi_approx = 0.0L;
    const long long pi_iterations = 1'000'000'000;
    std::cout << "Nota: O cálculo de PI será feito com " << pi_iterations << " iterações para maximizar a precisão.\n";
    measure_execution("d) Calcular PI com a aproximação de Leibniz", [&]() {
        pi_approx = calculate_pi_leibniz(pi_iterations);
        });
    std::cout << std::fixed << std::setprecision(18);
    std::cout << "[Resultado]: PI aproximado = " << pi_approx << "\n\n";

    return 0;
}