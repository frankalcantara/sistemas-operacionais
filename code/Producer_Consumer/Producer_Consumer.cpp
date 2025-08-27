/**
 * @file prime_calculator.cpp
 * @brief Sistema Producer-Consumer para calculo de numeros primos
 * @author Sistema Calculadora de Primos
 * @date 2025
 * @version 1.0
 *
 * Sistema multi-threaded que implementa o padrao Producer-Consumer
 * para calcular numeros primos em intervalos definidos, demonstrando
 * conceitos de sincronizacao de threads, buffers compartilhados e
 * balanceamento de carga computacional.
 */
 // Configuracoes para Windows
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <string>
#include <sstream>
#include <ctime>  // Para localtime_s

/**
 * @struct Interval
 * @brief Estrutura que representa um intervalo de numeros para processamento
 *
 * Esta estrutura define um intervalo de numeros que sera processado
 * pelas threads consumidoras para verificacao de primalidade.
 */
struct Interval {
    int start; ///< Numero inicial do intervalo (inclusivo)
    int end; ///< Numero final do intervalo (inclusivo)
    int id; ///< Identificador unico do intervalo
    /**
     * @brief Construtor padrao
     */
    Interval() : start(0), end(0), id(0) {}
    /**
     * @brief Construtor parametrizado
     * @param s Numero inicial do intervalo
     * @param e Numero final do intervalo
     * @param i Identificador do intervalo
     */
    Interval(int s, int e, int i) : start(s), end(e), id(i) {}
};


/**
 * @class IntervalBuffer
 * @brief Buffer thread-safe para armazenar intervalos de processamento
 *
 * Implementa um buffer circular thread-safe usando mutex e condition variables
 * para sincronizar o acesso entre threads produtoras e consumidoras.
 */
class IntervalBuffer {
private:
    std::queue<Interval> buffer_; ///< Fila de intervalos
    mutable std::mutex mtx_; ///< Mutex para proteger acesso ao buffer
    std::condition_variable cv_not_full_; ///< Condition variable para buffer nao cheio
    std::condition_variable cv_not_empty_; ///< Condition variable para buffer nao vazio
    size_t max_size_; ///< Tamanho maximo do buffer
    std::atomic<bool> shutdown_; ///< Flag para sinalizar shutdown
public:
    /**
     * @brief Construtor do buffer
     * @param max_size Tamanho maximo do buffer
     */
    explicit IntervalBuffer(size_t max_size)
        : max_size_(max_size), shutdown_(false) {
    }
    /**
     * @brief Adiciona um intervalo ao buffer
     * @param item Intervalo a ser adicionado
     * @return true se o item foi adicionado, false se houve shutdown
     *
     * Bloqueia se o buffer estiver cheio ate que haja espaco disponivel
     * ou ate que seja sinalizado o shutdown.
     */
    bool push(const Interval& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        // Espera ate que haja espaco ou shutdown seja sinalizado
        cv_not_full_.wait(lock, [this] {
            return buffer_.size() < max_size_ || shutdown_.load();
            });
        if (shutdown_.load()) {
            return false;
        }
        buffer_.push(item);
        cv_not_empty_.notify_one();
        return true;
    }
    /**
     * @brief Remove um intervalo do buffer
     * @param item Referencia para armazenar o intervalo removido
     * @return true se um item foi removido, false se buffer vazio e shutdown
     *
     * Bloqueia se o buffer estiver vazio ate que haja um item disponivel
     * ou ate que seja sinalizado o shutdown.
     */
    bool pop(Interval& item) {
        std::unique_lock<std::mutex> lock(mtx_);
        // Espera ate que haja item disponivel
        cv_not_empty_.wait(lock, [this] {
            return !buffer_.empty() || shutdown_.load();
            });
        if (buffer_.empty()) {
            return false;
        }
        item = buffer_.front();
        buffer_.pop();
        cv_not_full_.notify_one();
        return true;
    }
    /**
     * @brief Sinaliza o shutdown do buffer
     *
     * Acorda todas as threads bloqueadas em operacoes push/pop
     * para que possam finalizar graciosamente.
     */
    void signal_shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            shutdown_.store(true);
        }
        cv_not_full_.notify_all();
        cv_not_empty_.notify_all();
    }
    /**
     * @brief Retorna o tamanho atual do buffer
     * @return Numero de itens no buffer
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return buffer_.size();
    }
    /**
     * @brief Verifica se o shutdown foi sinalizado
     * @return true se shutdown foi sinalizado
     */
    bool is_shutdown() const {
        return shutdown_.load();
    }
};

/**
 * @struct Statistics
 * @brief Estrutura para coletar estatisticas do processamento
 *
 * Contem contadores atomicos para acompanhar o progresso do processamento
 * de forma thread-safe.
 */
struct Statistics {
    std::atomic<int> intervals_processed{ 0 }; ///< Numero de intervalos processados
    std::atomic<int> primes_found{ 0 }; ///< Numero total de primos encontrados
    std::atomic<long long> total_numbers_checked{ 0 }; ///< Total de numeros verificados
    std::chrono::steady_clock::time_point start_time; ///< Tempo de inicio do processamento
    mutable std::mutex last_primes_mutex; ///< Mutex para proteger last_primes
    std::vector<int> last_primes; ///< Ultimos primos encontrados (max 10)
    /**
     * @brief Construtor que inicializa o tempo de inicio
     */
    Statistics() : start_time(std::chrono::steady_clock::now()) {}
    /**
     * @brief Adiciona um primo a lista de ultimos primos encontrados
     * @param prime Numero primo a ser adicionado
     */
    void add_prime(int prime) {
        std::lock_guard<std::mutex> lock(last_primes_mutex);
        last_primes.push_back(prime);
        if (last_primes.size() > 10) {
            last_primes.erase(last_primes.begin());
        }
    }
    /**
     * @brief Retorna uma copia dos ultimos primos encontrados
     * @return Vetor com os ultimos primos
     */
    std::vector<int> get_last_primes() const {
        std::lock_guard<std::mutex> lock(last_primes_mutex);
        return last_primes;
    }
};

/**
 * @class PrimeCalculator
 * @brief Classe principal que coordena o sistema Producer-Consumer
 *
 * Gerencia as threads produtoras, consumidoras e de monitoramento,
 * coordenando todo o processo de calculo de numeros primos.
 */
class PrimeCalculator {
private:
    static constexpr int RANGE_START = 0; ///< Inicio do range de numeros
    static constexpr int RANGE_END = 100000000; ///< Fim do range de numeros
    static constexpr int INTERVAL_SIZE = 1000; ///< Tamanho de cada intervalo
    static constexpr int BUFFER_SIZE = 100; ///< Tamanho do buffer
    static constexpr int NUM_PRODUCERS = 16; ///< Numero de threads produtoras
    static constexpr int NUM_CONSUMERS = 8; ///< Numero de threads consumidoras
    IntervalBuffer buffer_; ///< Buffer compartilhado
    Statistics stats_; ///< Estatisticas do processamento
    std::vector<std::thread> producer_threads_; ///< Threads produtoras
    std::vector<std::thread> consumer_threads_; ///< Threads consumidoras
    std::thread monitor_thread_; ///< Thread de monitoramento
    std::atomic<bool> processing_complete_; ///< Flag de processamento completo
public:
    /**
     * @brief Construtor do calculador de primos
     */
    PrimeCalculator() : buffer_(BUFFER_SIZE), processing_complete_(false) {}
    /**
     * @brief Verifica se um numero e primo usando trial division
     * @param n Numero a ser verificado
     * @return true se o numero e primo
     *
     * Implementa o algoritmo de trial division de forma otimizada,
     * verificando divisibilidade apenas por numeros impares ate raiz de n.
     */
    static bool is_prime(int n) {
        if (n < 2) return false;
        if (n == 2) return true;
        if (n % 2 == 0) return false;
        // Trial division ate raiz de n (metodo custoso intencionalmente)
        for (int i = 3; i * i <= n; i += 2) {
            if (n % i == 0) return false;
        }
        return true;
    }
    /**
     * @brief Funcao executada pelas threads produtoras
     * @param producer_id ID da thread produtora
     *
     * Divide o range total em intervalos e os adiciona ao buffer.
     * Implementa rate limiting para evitar sobrecarga do sistema.
     */
    void producer_function(int producer_id) {
        const int total_intervals = (RANGE_END - RANGE_START + INTERVAL_SIZE - 1) / INTERVAL_SIZE;
        int interval_id = 0;
        for (int start = RANGE_START; start < RANGE_END; start += INTERVAL_SIZE * NUM_PRODUCERS) {
            int adjusted_start = start + (producer_id * INTERVAL_SIZE);
            if (adjusted_start >= RANGE_END) break;
            int end = std::min(adjusted_start + INTERVAL_SIZE - 1, RANGE_END - 1);
            Interval interval(adjusted_start, end, ++interval_id);
            if (!buffer_.push(interval)) {
                break; // Shutdown sinalizado
            }
            // Rate limiting - pequeno delay
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::cout << "Produtor " << producer_id << " finalizou\n";
    }
    /**
     * @brief Funcao executada pelas threads consumidoras
     * @param consumer_id ID da thread consumidora
     *
     * Remove intervalos do buffer e verifica a primalidade de cada numero
     * no intervalo, coletando estatisticas do processamento.
     */
    void consumer_function(int consumer_id) {
        Interval interval;
        while (buffer_.pop(interval)) {
            auto interval_start_time = std::chrono::high_resolution_clock::now();
            for (int num = interval.start; num <= interval.end; ++num) {
                auto calc_start = std::chrono::high_resolution_clock::now();
                bool prime = is_prime(num);
                auto calc_end = std::chrono::high_resolution_clock::now();
                auto calc_time = std::chrono::duration<double>(calc_end - calc_start);
                if (prime) {
                    ++stats_.primes_found;
                    stats_.add_prime(num);
                }
                ++stats_.total_numbers_checked;
            }
            ++stats_.intervals_processed;
            auto interval_end_time = std::chrono::high_resolution_clock::now();
            auto interval_duration = std::chrono::duration<double>(interval_end_time - interval_start_time);
        }
        std::cout << "Consumidor " << consumer_id << " finalizou\n";
    }
    /**
     * @brief Funcao de monitoramento que exibe estatisticas em tempo real
     *
     * Exibe periodicamente o progresso do processamento, incluindo
     * taxa de processamento, numeros primos encontrados e status do buffer.
     */
    void monitor_function() {
        const int total_intervals = (RANGE_END - RANGE_START + INTERVAL_SIZE - 1) / INTERVAL_SIZE;
        while (!processing_complete_.load()) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(current_time - stats_.start_time).count();
            int processed = stats_.intervals_processed.load();
            int primes = stats_.primes_found.load();
            long long checked = stats_.total_numbers_checked.load();
            size_t buffer_size = buffer_.size();
            // Cria barra de progresso para o buffer
            std::string buffer_bar = "[";
            int bar_width = 10;
            int filled = (buffer_size * bar_width) / BUFFER_SIZE;
            for (int i = 0; i < bar_width; ++i) {
                buffer_bar += (i < filled) ? "#" : " ";
            }
            buffer_bar += "]";
            // Calcula taxa de processamento
            double rate = (elapsed > 0) ? checked / elapsed : 0;
            double avg_time = (checked > 0) ? (elapsed * 1000) / checked : 0;
            // Formata tempo atual
            auto now = std::chrono::system_clock::now();
            auto time_t_var = std::chrono::system_clock::to_time_t(now);
            std::tm tm_struct;
            if (localtime_s(&tm_struct, &time_t_var) != 0) {
                std::cerr << "Erro ao converter tempo local. Usando valor padrão.\n";
                // Opcional: Continue ou use UTC
                continue;
            }
            auto tm = tm_struct;  // Use como antes
            // Obtém últimos primos encontrados
            auto last_primes = stats_.get_last_primes();
            // Limpa o terminal inteiro e move o cursor para o topo
            std::cout << "\033[2J\033[H";
            // Exibe estatísticas em um bloco coeso, com timestamp apenas no topo
            std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] Estatísticas de Processamento:\n"
                << "Buffer: " << buffer_bar << " " << buffer_size << "/" << BUFFER_SIZE
                << " (" << std::fixed << std::setprecision(1)
                << (100.0 * buffer_size / BUFFER_SIZE) << "%)\n"
                << "Processados: " << processed << " intervalos ("
                << std::fixed << std::setprecision(1) << (100.0 * processed / total_intervals) << "%)\n"
                << "Primos encontrados: " << primes << "\n"
                << "Taxa: " << std::fixed << std::setprecision(0) << rate << " números/segundo\n"
                << "Tempo médio: " << std::fixed << std::setprecision(1) << avg_time << "ms por número\n";
            // Exibe últimos primos
            if (!last_primes.empty()) {
                std::cout << "Últimos primos encontrados: ";
                for (size_t i = 0; i < last_primes.size() && i < 4; ++i) {
                    if (i > 0) std::cout << " | ";
                    std::cout << std::to_string(last_primes[i]);
                }
                std::cout << "\n";
            }
            std::cout << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    /**
     * @brief Executa o sistema completo de calculo de primos
     *
     * Coordena a criacao e execucao de todas as threads, exibe
     * informacoes iniciais e finais do processamento.
     */
    void run() {
        const int total_intervals = (RANGE_END - RANGE_START + INTERVAL_SIZE - 1) / INTERVAL_SIZE;
        // Exibe cabecalho
        std::cout << "=== CALCULADORA DE NUMEROS PRIMOS ===\n";
        std::cout << "Range: " << RANGE_START << " - " << RANGE_END
            << " | Intervalos: " << total_intervals
            << " | Produtores: " << NUM_PRODUCERS
            << " | Consumidores: " << NUM_CONSUMERS << "\n\n";
        // Inicia threads produtoras
        for (int i = 0; i < NUM_PRODUCERS; ++i) {
            producer_threads_.emplace_back(&PrimeCalculator::producer_function, this, i);
        }
        // Inicia threads consumidoras
        for (int i = 0; i < NUM_CONSUMERS; ++i) {
            consumer_threads_.emplace_back(&PrimeCalculator::consumer_function, this, i);
        }
        // Inicia thread de monitoramento
        monitor_thread_ = std::thread(&PrimeCalculator::monitor_function, this);
        // Aguarda produtores terminarem
        for (auto& t : producer_threads_) {
            t.join();
        }
        // Sinaliza shutdown do buffer
        buffer_.signal_shutdown();
        // Aguarda consumidores terminarem
        for (auto& t : consumer_threads_) {
            t.join();
        }
        // Sinaliza fim do processamento e aguarda monitor
        processing_complete_.store(true);
        monitor_thread_.join();
        // Exibe resultados finais
        auto end_time = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration<double>(end_time - stats_.start_time).count();
        std::cout << "\n[PROCESSAMENTO COMPLETO!]\n";
        std::cout << "Total de primos no range: " << stats_.primes_found.load() << "\n";
        std::cout << "Tempo total: " << std::fixed << std::setprecision(1) << total_time << " segundos\n";
        std::cout << "Numeros verificados: " << stats_.total_numbers_checked.load() << "\n";
        std::cout << "Taxa media: " << std::fixed << std::setprecision(0)
            << (stats_.total_numbers_checked.load() / total_time) << " numeros/segundo\n";
    }
};

/**
 * @brief Configura o console para suporte a UTF-8 no Windows
 *
 * Esta funcao configura o console do Windows para exibir
 * caracteres UTF-8 corretamente.
 */
void setup_console() {
#ifdef _WIN32
    // Configura o console para UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // Habilita processamento de escape sequences no Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

/**
 * @brief Funcao principal do programa
 * @return 0 se execucao bem-sucedida
 *
 * Cria uma instancia do PrimeCalculator e executa o sistema
 * de calculo de numeros primos.
 */
int main() {
    try {
        setup_console();
        PrimeCalculator calculator;
        calculator.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}