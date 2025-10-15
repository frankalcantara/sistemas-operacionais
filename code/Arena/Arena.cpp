/**
 * @file main.cpp
 * @author Frank Alcantara
 * @brief Comparação da Alocação por Arena vs. Heap.
 * @version 1.2
 * @date 2025-10-09
 *
 * @copyright Copyright (c) 2025
 *
 * @details
 * Este programa implementa e compara duas estratégias de gerenciamento de memória
 * para benchmark de alocação normal, heap e arena, para isso usamos uma simulação de edição de grafos
 * e duas formas de alocação:
 * 1. Alocação Padrão no Heap (usando `new` e `delete`).
 * 2. Alocação por Arena (usando um alocador customizado).
 *
 * O objetivo é demonstrar a diferença de desempenho entre as duas abordagens,
 * explicando os conceitos subjacentes de forma clara.
 *
 * Testado no Visual Studio 2022 (v17.5) com C++23 sobre o Windows 11.
 * 
 * O código tem a seguinte estrutura:
 * +--------------------------+
*  |        main.cpp          |
*  |  Comparação Arena vs     |
*  |          Heap            |
*  +--------------------------+
*              |
*              v
*  +-----------------------------------+
*  |        Estruturas de Dados        |
*  |  - Vetor2D (POD)                  |
*  |  - PropriedadesVisuais (POD)      |
*  |  - Aresta (com No*)               |
*  |  - No (com Vetor2D, props, nome)  |
*  +-----------------------------------+
*              |
*              v
*  +-----------------------------------+
*  |        Classe Arena               |
*  |  - Ctor: aloca bloco (std::byte)  |
*  |  - Dtor: delete[] bloco           |
*  |  - alocar(size_t): avança ptr     |
*  |    (com alinhamento 16 bytes)     |
*  |  - resetar(): volta ao início     |
*  +-----------------------------------+
*              |
*              v
*  +------------------------------------------------+
*  |        Funções de Simulação                    |
*  |  - setup_random_engine(): MT19937              |
*  |  - simulacao_heap(gen):                        |
*  |    new No/Aresta -> delete loops               |
*  |  - simulacao_arena(gen):                       |
*  |    arena.alocar() -> placement new -> resetar()|
*  +------------------------------------------------+
*                       |
*                       v
*  +-------------------------------------------+
*  |          Benchmark no main                |
*  |  - gen = setup_random_engine()            |
*  |  - chrono: start_heap -> heap()           |
*  |  - chrono: start_arena -> arena()         |        
*  |  - Calcula speedup: heap/arena            |
*  |  - Output: tempos e ratio (x mais rápida) |
*  +-------------------------------------------+
*
*/
#include <iostream>
#include <vector>
#include <chrono> // Para medição de tempo de alta precisão.
#include <random> // Para geração de números aleatórios na simulação.
#include <cstddef> // Para std::byte e size_t.
#include <string> // Usado indiretamente, mas boa prática incluir.
#include <new> // Para placement new. Essencial para o alocador de arena.
#include <numeric> // Para std::iota (não usado na versão final, mas útil em debug).
#include <clocale> // Para setlocale, garantindo a correta exibição de caracteres.
#include <cstdio> // Para snprintf, uma forma segura de formatar strings C-style.
#include <iomanip> // Para std::fixed e std::setprecision na exibição dos resultados.

// --- ESTRUTURAS DE DADOS DO PROBLEMA ---
// Estas são estruturas simples, conhecidas como PODs (Plain Old Data).
// Elas não possuem construtores, destrutores ou membros virtuais complexos,
// o que as torna ideais para alocação em uma arena, pois a "destruição"
// pode ser simplesmente ignorar a memória que ocupavam.

/** @struct Vetor2D
  * @brief Representa um ponto ou vetor no espaço 2D.
*/
struct Vetor2D {
    float x, y;
};
/** @struct PropriedadesVisuais
 * @brief Armazena atributos visuais de um objeto, como um nó de grafo.
 */
struct PropriedadesVisuais {
    int cor_rgb;
    float tamanho;
};
// Forward declaration de 'No' é necessária porque a struct 'Aresta'
// contém ponteiros para 'No', mas 'No' é definida depois.
struct No;
/** @struct Aresta
 * @brief Representa a conexão entre dois nós em um grafo.
 */
struct Aresta {
    int id;
    No* origem;
    No* destino;
    float peso;
};
/** @struct No
 * @brief Representa um nó (ou vértice) em um grafo.
 */
struct No {
    int id;
    Vetor2D posicao;
    PropriedadesVisuais props;
    char nome[16]; // Usamos um array de char C-style para manter a estrutura como POD.
};

// --- ETAPA 1: IMPLEMENTAÇÃO DO ARENA ALLOCATOR ---
/**
 * @class Arena
 * @brief Implementa um alocador de memória baseado na técnica de Arena (Region-based).
 *
 * @details
 * A ideia central é alocar um único e grande bloco de memória (a "arena") do sistema
 * operacional de uma só vez. As alocações subsequentes para objetos individuais são
 * atendidas a partir deste bloco, simplesmente incrementando um ponteiro.
 * Isso é extremamente rápido, pois evita a sobrecarga de múltiplas chamadas ao
 * alocador do sistema (como `malloc` ou o `new` do SO) e a complexidade de
 * gerenciar listas de blocos livres.
 */
class Arena {
public:
    /**
     * @brief Construtor que aloca o bloco de memória principal para a arena.
     * @param tamanho_bytes O tamanho total da arena a ser alocada, em bytes.
     */
    explicit Arena(size_t tamanho_bytes)
        : m_tamanho_total(tamanho_bytes) {
        // Alocamos um buffer de bytes brutos. `std::byte` é usado para indicar
        // que esta é uma memória não tipada, o que é uma boa prática em C++ moderno.
        // `std::nothrow` evita que uma exceção seja lançada em caso de falha,
        // permitindo-nos tratar o erro manualmente (retornando nullptr).
        m_inicio = new (std::nothrow) std::byte[tamanho_bytes];
        if (!m_inicio) {
            throw std::bad_alloc();
        }
        m_fim = m_inicio + tamanho_bytes;
        m_atual = m_inicio;
    }
    /**
     * @brief Destrutor que libera o bloco de memória da arena.
     */
    ~Arena() {
        // Apenas uma chamada a `delete[]` é necessária para liberar toda a memória
        // que foi usada por milhares de objetos.
        delete[] m_inicio;
    }
    // Proibimos a cópia e a atribuição para seguir o princípio de RAII (Resource
    // Acquisition Is Initialization) e evitar problemas como "double-free", onde
    // duas instâncias de Arena poderiam tentar deletar o mesmo ponteiro `m_inicio`.
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    /**
     * @brief Aloca um bloco de memória dentro da arena.
     * @param tamanho_bytes O número de bytes a serem alocados.
     * @return Um ponteiro `void*` para o início do bloco alocado, ou `nullptr` se não houver espaço.
     *
     * @note Esta implementação foi aprimorada para fins educacionais, incluindo alinhamento
     * de memória básico (para 16 bytes, comum em arquiteturas modernas). Em um sistema real,
     * seria preciso garantir que o ponteiro retornado seja alinhado corretamente para o tipo de
     * objeto que será armazenado nele. A falta de alinhamento pode causar
     * quedas drásticas de desempenho ou até mesmo falhas em algumas arquiteturas.
     */
    void* alocar(size_t tamanho_bytes) {
        if (m_atual + tamanho_bytes > m_fim) {
            return nullptr; // Fim do espaço disponível na arena.
        }
        // Alinhar o ponteiro atual para múltiplo de 16 bytes (exemplo para alinhamento comum).
        // Isso garante que o ponteiro seja adequado para a maioria dos tipos POD.
        uintptr_t ptr_atual = reinterpret_cast<uintptr_t>(m_atual);
        size_t offset_alinhamento = (16 - (ptr_atual % 16)) % 16;
        std::byte* ptr_alinhado = m_atual + offset_alinhamento;
        if (ptr_alinhado + tamanho_bytes > m_fim) {
            return nullptr; // Não há espaço suficiente após o alinhamento.
        }
        void* ptr = ptr_alinhado;
        // A "alocação" é simplesmente avançar o ponteiro.
        // Esta é uma operação aritmeticamente muito barata.
        m_atual = ptr_alinhado + tamanho_bytes;
        return ptr;
    }
    /**
     * @brief Reseta a arena, tornando toda a sua memória disponível novamente.
     *
     * @details Esta operação é o "coração" da eficiência da arena para desalocação.
     * Em vez de desalocar cada objeto individualmente, nós simplesmente
     * reiniciamos o ponteiro `m_atual` para o início da arena.
     *
     * @warning Este método NÃO chama os destrutores dos objetos que foram alocados
     * na arena. Portanto, esta técnica só é segura para objetos que
     * não gerenciam outros recursos (como arquivos, sockets, ou outra
     * memória alocada no heap), ou seja, objetos POD ou com destrutores triviais.
     */
    void resetar() {
        m_atual = m_inicio;
    }
private:
    size_t m_tamanho_total; ///< Tamanho total da arena em bytes.
    std::byte* m_inicio = nullptr; ///< Ponteiro para o início do bloco de memória.
    std::byte* m_fim = nullptr; ///< Ponteiro para o fim do bloco de memória.
    std::byte* m_atual = nullptr; ///< Ponteiro para a próxima posição livre (o "topo" da pilha).
};

// --- FUNÇÕES DE SIMULAÇÃO ---
// Constantes para a simulação.
constexpr int NUMERO_ITERACOES = 1000;
constexpr int MIN_NOS = 500;
constexpr int MAX_NOS = 1000;
constexpr int MIN_ARESTAS = 1000;
constexpr int MAX_ARESTAS = 2000;
constexpr size_t TAMANHO_ARENA = 1024 * 1024 * 50; // 50 MB, um tamanho generoso.

/**
 * @brief Configura e retorna um gerador de números aleatórios de alta qualidade.
 * @return Uma instância de std::mt19937.
 */
std::mt19937 setup_random_engine() {
    std::random_device rd;
    return std::mt19937(rd());
}
/**
 * @brief ETAPA 2: Executa a simulação alocando e desalocando objetos no Heap.
 *
 * @details Esta função simula o ciclo de vida dos objetos usando o alocador
 * padrão do C++ (`new` e `delete`). Cada chamada a `new` pode resultar em uma
 * chamada ao sistema operacional para encontrar um bloco de memória, o que
 * tem uma sobrecarga de desempenho. Da mesma forma, cada `delete` precisa
 * informar ao sistema que a memória está livre, o que também custa tempo.
 * O acúmulo de milhares dessas operações é o que torna essa abordagem lenta.
 *
 * @param gen Referência para o gerador de números aleatórios, otimizando o uso ao evitar recriações desnecessárias.
 */
void simulacao_heap(std::mt19937& gen) {
    std::uniform_int_distribution<> dist_nos(MIN_NOS, MAX_NOS);
    std::uniform_int_distribution<> dist_arestas(MIN_ARESTAS, MAX_ARESTAS);
    for (int i = 0; i < NUMERO_ITERACOES; ++i) {
        std::vector<No*> nos;
        std::vector<Aresta*> arestas;
        int num_nos = dist_nos(gen);
        nos.reserve(num_nos);
        for (int j = 0; j < num_nos; ++j) {
            // Alocação individual no heap. Custo computacional incorrido aqui.
            No* no = new No{ .id = j, .posicao = { (float)j, (float)j }, .props = { 0xFF0000, 10.0f } };
            snprintf(no->nome, sizeof(no->nome), "No_%d", j);
            nos.push_back(no);
        }
        if (num_nos > 1) {
            std::uniform_int_distribution<> dist_indices_nos(0, num_nos - 1);
            int num_arestas = dist_arestas(gen);
            arestas.reserve(num_arestas);
            for (int j = 0; j < num_arestas; ++j) {
                // Outra alocação individual no heap.
                Aresta* aresta = new Aresta{ .id = j, .origem = nos[dist_indices_nos(gen)], .destino = nos[dist_indices_nos(gen)], .peso = (float)j };
                arestas.push_back(aresta);
            }
        }
        // Desalocação individual. Outro custo computacional incorrido em um loop.
        for (Aresta* a : arestas) {
            delete a;
        }
        for (No* n : nos) {
            delete n;
        }
    }
}
/**
 * @brief ETAPA 3: Executa a simulação usando o Arena Allocator.
 *
 * @details Esta versão usa a Arena. Em vez de chamar `new` para cada objeto,
 * pedimos um pedaço de memória à arena (`arena.alocar()`) e então usamos
 * o "placement new" para construir o objeto nesse local. O "placement new"
 * (`new (ponteiro) Tipo(args)`) não aloca memória; ele apenas chama o
 * construtor do objeto em um endereço de memória já alocado.
 * A desalocação é feita em uma única chamada a `arena.resetar()`, que é
 * ordens de magnitude mais rápida do que deletar cada objeto individualmente.
 *
 * @param gen Referência para o gerador de números aleatórios, otimizando o uso ao evitar recriações desnecessárias.
 */
void simulacao_arena(std::mt19937& gen) {
    std::uniform_int_distribution<> dist_nos(MIN_NOS, MAX_NOS);
    std::uniform_int_distribution<> dist_arestas(MIN_ARESTAS, MAX_ARESTAS);
    Arena arena(TAMANHO_ARENA);
    for (int i = 0; i < NUMERO_ITERACOES; ++i) {
        std::vector<No*> nos;
        std::vector<Aresta*> arestas;
        int num_nos = dist_nos(gen);
        nos.reserve(num_nos);
        for (int j = 0; j < num_nos; ++j) {
            void* mem = arena.alocar(sizeof(No));
            if (!mem) {
                // Tratamento de erro: se não houver espaço suficiente, pulamos esta iteração
                // para evitar comportamento indefinido. Em um sistema real, poderia-se
                // usar uma arena maior ou fallback para heap.
                std::cerr << "Aviso: Falha na alocação na arena na iteração " << i << ", pulando.\n";
                return; // Sai da função para simplicidade didática; em produção, continue ou ajuste.
            }
            // "Placement new": constrói o objeto na memória fornecida pela arena.
            No* no = new (mem) No{ .id = j, .posicao = { (float)j, (float)j }, .props = { 0x00FF00, 10.0f } };
            snprintf(no->nome, sizeof(no->nome), "No_%d", j);
            nos.push_back(no);
        }
        if (num_nos > 1) {
            std::uniform_int_distribution<> dist_indices_nos(0, num_nos - 1);
            int num_arestas = dist_arestas(gen);
            arestas.reserve(num_arestas);
            for (int j = 0; j < num_arestas; ++j) {
                void* mem = arena.alocar(sizeof(Aresta));
                if (!mem) {
                    // Tratamento de erro similar para arestas.
                    std::cerr << "Aviso: Falha na alocação de aresta na iteração " << i << ", pulando.\n";
                    return;
                }
                Aresta* aresta = new (mem) Aresta{ .id = j, .origem = nos[dist_indices_nos(gen)], .destino = nos[dist_indices_nos(gen)], .peso = (float)j };
                arestas.push_back(aresta);
            }
        }
        // Desalocação de milhares de objetos em uma única operação de baixíssimo custo.
        arena.resetar();
    }
}
// --- ETAPA 4: BENCHMARKING E ANÁLISE ---
int main() {
    setlocale(LC_ALL, "pt_BR.UTF-8");
    std::cout << "Iniciando benchmark de alocacao de memoria...\n";
    std::cout << "Numero de iteracoes por teste: " << NUMERO_ITERACOES << "\n\n";
    // Configuramos o gerador de números aleatórios uma única vez para otimizar o benchmark,
    // evitando recriações desnecessárias em cada simulação.
    auto gen = setup_random_engine();
    // Medindo o tempo da simulação com Heap
    std::cout << "Executando simulacao com alocacao no HEAP..." << std::flush;
    auto start_heap = std::chrono::high_resolution_clock::now();
    simulacao_heap(gen);
    auto end_heap = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tempo_heap = end_heap - start_heap;
    std::cout << " CONCLUIDO.\n";
    // Medindo o tempo da simulação com Arena
    std::cout << "Executando simulacao com alocacao por ARENA..." << std::flush;
    auto start_arena = std::chrono::high_resolution_clock::now();
    simulacao_arena(gen);
    auto end_arena = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tempo_arena = end_arena - start_arena;
    std::cout << " CONCLUIDO.\n\n";
    // Exibindo resultados
    std::cout << "--- Resultados do Benchmark ---\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Tempo total com Heap: " << tempo_heap.count() << " ms\n";
    std::cout << "Tempo total com Arena: " << tempo_arena.count() << " ms\n";
     std::cout << "-------------------------------\n\n";
    if (tempo_heap.count() > 0 && tempo_arena.count() > 0) {
        double diferenca = tempo_heap.count() / tempo_arena.count();
        std::cout << "A alocacao por Arena foi aproximadamente " << diferenca << "x mais rapida que a alocacao no Heap para esta carga de trabalho.\n";
    }
    return 0;
}