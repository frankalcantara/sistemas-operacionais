/**
 * @file main.cpp
 * @author Frank Alcantara
 * @brief Compara��o da Aloca��o por Arena vs. Heap.
 * @version 1.2
 * @date 2025-10-09
 *
 * @copyright Copyright (c) 2025
 *
 * @details
 * Este programa implementa e compara duas estrat�gias de gerenciamento de mem�ria
 * para benchmark de aloca��o normal, heap e arena, para isso usamos uma simula��o de edi��o de grafos
 * e duas formas de aloca��o:
 * 1. Aloca��o Padr�o no Heap (usando `new` e `delete`).
 * 2. Aloca��o por Arena (usando um alocador customizado).
 *
 * O objetivo � demonstrar a diferen�a de desempenho entre as duas abordagens,
 * explicando os conceitos subjacentes de forma clara.
 *
 * Testado no Visual Studio 2022 (v17.5) com C++23 sobre o Windows 11.
 * 
 * O c�digo tem a seguinte estrutura:
 * +--------------------------+
*  |        main.cpp          |
*  |  Compara��o Arena vs     |
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
*  |  - alocar(size_t): avan�a ptr     |
*  |    (com alinhamento 16 bytes)     |
*  |  - resetar(): volta ao in�cio     |
*  +-----------------------------------+
*              |
*              v
*  +------------------------------------------------+
*  |        Fun��es de Simula��o                    |
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
*  |  - Output: tempos e ratio (x mais r�pida) |
*  +-------------------------------------------+
*
*/
#include <iostream>
#include <vector>
#include <chrono> // Para medi��o de tempo de alta precis�o.
#include <random> // Para gera��o de n�meros aleat�rios na simula��o.
#include <cstddef> // Para std::byte e size_t.
#include <string> // Usado indiretamente, mas boa pr�tica incluir.
#include <new> // Para placement new. Essencial para o alocador de arena.
#include <numeric> // Para std::iota (n�o usado na vers�o final, mas �til em debug).
#include <clocale> // Para setlocale, garantindo a correta exibi��o de caracteres.
#include <cstdio> // Para snprintf, uma forma segura de formatar strings C-style.
#include <iomanip> // Para std::fixed e std::setprecision na exibi��o dos resultados.

// --- ESTRUTURAS DE DADOS DO PROBLEMA ---
// Estas s�o estruturas simples, conhecidas como PODs (Plain Old Data).
// Elas n�o possuem construtores, destrutores ou membros virtuais complexos,
// o que as torna ideais para aloca��o em uma arena, pois a "destrui��o"
// pode ser simplesmente ignorar a mem�ria que ocupavam.

/** @struct Vetor2D
  * @brief Representa um ponto ou vetor no espa�o 2D.
*/
struct Vetor2D {
    float x, y;
};
/** @struct PropriedadesVisuais
 * @brief Armazena atributos visuais de um objeto, como um n� de grafo.
 */
struct PropriedadesVisuais {
    int cor_rgb;
    float tamanho;
};
// Forward declaration de 'No' � necess�ria porque a struct 'Aresta'
// cont�m ponteiros para 'No', mas 'No' � definida depois.
struct No;
/** @struct Aresta
 * @brief Representa a conex�o entre dois n�s em um grafo.
 */
struct Aresta {
    int id;
    No* origem;
    No* destino;
    float peso;
};
/** @struct No
 * @brief Representa um n� (ou v�rtice) em um grafo.
 */
struct No {
    int id;
    Vetor2D posicao;
    PropriedadesVisuais props;
    char nome[16]; // Usamos um array de char C-style para manter a estrutura como POD.
};

// --- ETAPA 1: IMPLEMENTA��O DO ARENA ALLOCATOR ---
/**
 * @class Arena
 * @brief Implementa um alocador de mem�ria baseado na t�cnica de Arena (Region-based).
 *
 * @details
 * A ideia central � alocar um �nico e grande bloco de mem�ria (a "arena") do sistema
 * operacional de uma s� vez. As aloca��es subsequentes para objetos individuais s�o
 * atendidas a partir deste bloco, simplesmente incrementando um ponteiro.
 * Isso � extremamente r�pido, pois evita a sobrecarga de m�ltiplas chamadas ao
 * alocador do sistema (como `malloc` ou o `new` do SO) e a complexidade de
 * gerenciar listas de blocos livres.
 */
class Arena {
public:
    /**
     * @brief Construtor que aloca o bloco de mem�ria principal para a arena.
     * @param tamanho_bytes O tamanho total da arena a ser alocada, em bytes.
     */
    explicit Arena(size_t tamanho_bytes)
        : m_tamanho_total(tamanho_bytes) {
        // Alocamos um buffer de bytes brutos. `std::byte` � usado para indicar
        // que esta � uma mem�ria n�o tipada, o que � uma boa pr�tica em C++ moderno.
        // `std::nothrow` evita que uma exce��o seja lan�ada em caso de falha,
        // permitindo-nos tratar o erro manualmente (retornando nullptr).
        m_inicio = new (std::nothrow) std::byte[tamanho_bytes];
        if (!m_inicio) {
            throw std::bad_alloc();
        }
        m_fim = m_inicio + tamanho_bytes;
        m_atual = m_inicio;
    }
    /**
     * @brief Destrutor que libera o bloco de mem�ria da arena.
     */
    ~Arena() {
        // Apenas uma chamada a `delete[]` � necess�ria para liberar toda a mem�ria
        // que foi usada por milhares de objetos.
        delete[] m_inicio;
    }
    // Proibimos a c�pia e a atribui��o para seguir o princ�pio de RAII (Resource
    // Acquisition Is Initialization) e evitar problemas como "double-free", onde
    // duas inst�ncias de Arena poderiam tentar deletar o mesmo ponteiro `m_inicio`.
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    /**
     * @brief Aloca um bloco de mem�ria dentro da arena.
     * @param tamanho_bytes O n�mero de bytes a serem alocados.
     * @return Um ponteiro `void*` para o in�cio do bloco alocado, ou `nullptr` se n�o houver espa�o.
     *
     * @note Esta implementa��o foi aprimorada para fins educacionais, incluindo alinhamento
     * de mem�ria b�sico (para 16 bytes, comum em arquiteturas modernas). Em um sistema real,
     * seria preciso garantir que o ponteiro retornado seja alinhado corretamente para o tipo de
     * objeto que ser� armazenado nele. A falta de alinhamento pode causar
     * quedas dr�sticas de desempenho ou at� mesmo falhas em algumas arquiteturas.
     */
    void* alocar(size_t tamanho_bytes) {
        if (m_atual + tamanho_bytes > m_fim) {
            return nullptr; // Fim do espa�o dispon�vel na arena.
        }
        // Alinhar o ponteiro atual para m�ltiplo de 16 bytes (exemplo para alinhamento comum).
        // Isso garante que o ponteiro seja adequado para a maioria dos tipos POD.
        uintptr_t ptr_atual = reinterpret_cast<uintptr_t>(m_atual);
        size_t offset_alinhamento = (16 - (ptr_atual % 16)) % 16;
        std::byte* ptr_alinhado = m_atual + offset_alinhamento;
        if (ptr_alinhado + tamanho_bytes > m_fim) {
            return nullptr; // N�o h� espa�o suficiente ap�s o alinhamento.
        }
        void* ptr = ptr_alinhado;
        // A "aloca��o" � simplesmente avan�ar o ponteiro.
        // Esta � uma opera��o aritmeticamente muito barata.
        m_atual = ptr_alinhado + tamanho_bytes;
        return ptr;
    }
    /**
     * @brief Reseta a arena, tornando toda a sua mem�ria dispon�vel novamente.
     *
     * @details Esta opera��o � o "cora��o" da efici�ncia da arena para desaloca��o.
     * Em vez de desalocar cada objeto individualmente, n�s simplesmente
     * reiniciamos o ponteiro `m_atual` para o in�cio da arena.
     *
     * @warning Este m�todo N�O chama os destrutores dos objetos que foram alocados
     * na arena. Portanto, esta t�cnica s� � segura para objetos que
     * n�o gerenciam outros recursos (como arquivos, sockets, ou outra
     * mem�ria alocada no heap), ou seja, objetos POD ou com destrutores triviais.
     */
    void resetar() {
        m_atual = m_inicio;
    }
private:
    size_t m_tamanho_total; ///< Tamanho total da arena em bytes.
    std::byte* m_inicio = nullptr; ///< Ponteiro para o in�cio do bloco de mem�ria.
    std::byte* m_fim = nullptr; ///< Ponteiro para o fim do bloco de mem�ria.
    std::byte* m_atual = nullptr; ///< Ponteiro para a pr�xima posi��o livre (o "topo" da pilha).
};

// --- FUN��ES DE SIMULA��O ---
// Constantes para a simula��o.
constexpr int NUMERO_ITERACOES = 1000;
constexpr int MIN_NOS = 500;
constexpr int MAX_NOS = 1000;
constexpr int MIN_ARESTAS = 1000;
constexpr int MAX_ARESTAS = 2000;
constexpr size_t TAMANHO_ARENA = 1024 * 1024 * 50; // 50 MB, um tamanho generoso.

/**
 * @brief Configura e retorna um gerador de n�meros aleat�rios de alta qualidade.
 * @return Uma inst�ncia de std::mt19937.
 */
std::mt19937 setup_random_engine() {
    std::random_device rd;
    return std::mt19937(rd());
}
/**
 * @brief ETAPA 2: Executa a simula��o alocando e desalocando objetos no Heap.
 *
 * @details Esta fun��o simula o ciclo de vida dos objetos usando o alocador
 * padr�o do C++ (`new` e `delete`). Cada chamada a `new` pode resultar em uma
 * chamada ao sistema operacional para encontrar um bloco de mem�ria, o que
 * tem uma sobrecarga de desempenho. Da mesma forma, cada `delete` precisa
 * informar ao sistema que a mem�ria est� livre, o que tamb�m custa tempo.
 * O ac�mulo de milhares dessas opera��es � o que torna essa abordagem lenta.
 *
 * @param gen Refer�ncia para o gerador de n�meros aleat�rios, otimizando o uso ao evitar recria��es desnecess�rias.
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
            // Aloca��o individual no heap. Custo computacional incorrido aqui.
            No* no = new No{ .id = j, .posicao = { (float)j, (float)j }, .props = { 0xFF0000, 10.0f } };
            snprintf(no->nome, sizeof(no->nome), "No_%d", j);
            nos.push_back(no);
        }
        if (num_nos > 1) {
            std::uniform_int_distribution<> dist_indices_nos(0, num_nos - 1);
            int num_arestas = dist_arestas(gen);
            arestas.reserve(num_arestas);
            for (int j = 0; j < num_arestas; ++j) {
                // Outra aloca��o individual no heap.
                Aresta* aresta = new Aresta{ .id = j, .origem = nos[dist_indices_nos(gen)], .destino = nos[dist_indices_nos(gen)], .peso = (float)j };
                arestas.push_back(aresta);
            }
        }
        // Desaloca��o individual. Outro custo computacional incorrido em um loop.
        for (Aresta* a : arestas) {
            delete a;
        }
        for (No* n : nos) {
            delete n;
        }
    }
}
/**
 * @brief ETAPA 3: Executa a simula��o usando o Arena Allocator.
 *
 * @details Esta vers�o usa a Arena. Em vez de chamar `new` para cada objeto,
 * pedimos um peda�o de mem�ria � arena (`arena.alocar()`) e ent�o usamos
 * o "placement new" para construir o objeto nesse local. O "placement new"
 * (`new (ponteiro) Tipo(args)`) n�o aloca mem�ria; ele apenas chama o
 * construtor do objeto em um endere�o de mem�ria j� alocado.
 * A desaloca��o � feita em uma �nica chamada a `arena.resetar()`, que �
 * ordens de magnitude mais r�pida do que deletar cada objeto individualmente.
 *
 * @param gen Refer�ncia para o gerador de n�meros aleat�rios, otimizando o uso ao evitar recria��es desnecess�rias.
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
                // Tratamento de erro: se n�o houver espa�o suficiente, pulamos esta itera��o
                // para evitar comportamento indefinido. Em um sistema real, poderia-se
                // usar uma arena maior ou fallback para heap.
                std::cerr << "Aviso: Falha na aloca��o na arena na itera��o " << i << ", pulando.\n";
                return; // Sai da fun��o para simplicidade did�tica; em produ��o, continue ou ajuste.
            }
            // "Placement new": constr�i o objeto na mem�ria fornecida pela arena.
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
                    std::cerr << "Aviso: Falha na aloca��o de aresta na itera��o " << i << ", pulando.\n";
                    return;
                }
                Aresta* aresta = new (mem) Aresta{ .id = j, .origem = nos[dist_indices_nos(gen)], .destino = nos[dist_indices_nos(gen)], .peso = (float)j };
                arestas.push_back(aresta);
            }
        }
        // Desaloca��o de milhares de objetos em uma �nica opera��o de baix�ssimo custo.
        arena.resetar();
    }
}
// --- ETAPA 4: BENCHMARKING E AN�LISE ---
int main() {
    setlocale(LC_ALL, "pt_BR.UTF-8");
    std::cout << "Iniciando benchmark de alocacao de memoria...\n";
    std::cout << "Numero de iteracoes por teste: " << NUMERO_ITERACOES << "\n\n";
    // Configuramos o gerador de n�meros aleat�rios uma �nica vez para otimizar o benchmark,
    // evitando recria��es desnecess�rias em cada simula��o.
    auto gen = setup_random_engine();
    // Medindo o tempo da simula��o com Heap
    std::cout << "Executando simulacao com alocacao no HEAP..." << std::flush;
    auto start_heap = std::chrono::high_resolution_clock::now();
    simulacao_heap(gen);
    auto end_heap = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> tempo_heap = end_heap - start_heap;
    std::cout << " CONCLUIDO.\n";
    // Medindo o tempo da simula��o com Arena
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