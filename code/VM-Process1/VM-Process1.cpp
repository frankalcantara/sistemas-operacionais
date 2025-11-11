/**
 * @file main.cpp
 * @brief Implementação de uma Máquina Virtual baseada em pilha (Stack-based VM) em C++23.
 *
 * Este arquivo contém a definição e implementação de uma VM simples, projetada para fins didáticos.
 * A arquitetura utiliza uma pilha de operandos do tipo `std::int64_t` e suporta um conjunto
 * básico de instruções aritméticas e de controle de fluxo.
 *
 * @details
 * Funcionalidades principais:
 * - Validação de opcodes em tempo de compilação (quando possível) e execução.
 * - Tratamento de erros através de exceções tipadas (`VMError`).
 * - Suporte a *Endianness* configurável (Big-Endian ou Little-Endian).
 * - Modo de depuração integrado.
 *
 * Destaques de C++23 e Modern C++:
 * - Uso extensivo de `constexpr` para validação e estruturas de dados imutáveis.
 * - `enum class` para escopo forte e segurança de tipos nos Opcodes.
 * - `[[nodiscard]]` para garantir que retornos de funções puras não sejam ignorados.
 * - Lambdas para abstração de operações aritméticas.
 *
 * @author Frank Alcantara (gemini para comentários, claude e grok para correções)
 * @date 11/11/2025
 * @version 1.0
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <optional>
#include <iomanip>

 // =========================== Configurações e Tipos ===========================

 /**
  * @brief Tipo base para um byte de memória (8 bits).
  */
using Byte = uint8_t;

/**
 * @brief Tipo para representar uma palavra de 16 bits (usado em endereços e literais maiores).
 */
using Word16 = uint16_t;

/**
 * @brief Tipo base para os valores na pilha da VM.
 * @note Utiliza `std::int64_t` para garantir precisão em cálculos inteiros em arquiteturas de 64 bits.
 */
using Int = std::int64_t;

/**
 * @brief Tipo para endereçamento de memória (índice no vetor de bytes).
 */
using Address = std::size_t;

/**
 * @brief Enumeração para configuração da ordem dos bytes (Endianness).
 */
enum class Endianness { Big, Little };

/**
 * @class VMError
 * @brief Exceção específica da Máquina Virtual.
 *
 * Herda de `std::runtime_error` para encapsular erros de execução, como:
 * - Stack underflow.
 * - Opcode inválido.
 * - Acesso inválido à memória.
 * - Divisão por zero.
 */
class VMError : public std::runtime_error {
public:
    /**
     * @brief Construtor da exceção.
     * @param msg Mensagem de erro detalhada.
     */
    explicit VMError(std::string msg) : std::runtime_error("VM_ERROR: " + std::move(msg)) {}
};

/**
 * @enum Opcode
 * @brief Conjunto de instruções suportadas pela VM.
 *
 * Utiliza `enum class` herdando de `Byte` para garantir que cada instrução ocupe exatamente 1 byte
 * e para evitar conversões implícitas indesejadas.
 */
enum class Opcode : Byte {
    HALT = 0x00,   ///< Para a execução da VM.
    PUSH = 0x01,   ///< Empilha um valor de 8 bits: `PUSH <uint8_t>`.
    POP = 0x02,    ///< Desempilha o valor do topo e o descarta.
    ADD = 0x03,    ///< Soma os dois valores do topo: $a + b$.
    SUB = 0x04,    ///< Subtrai os dois valores do topo: $a - b$.
    MUL = 0x05,    ///< Multiplica os dois valores do topo: $a \times b$.
    DIV = 0x06,    ///< Divide os dois valores do topo: $a / b$.
    PRINT = 0x07,  ///< Imprime o valor do topo da pilha na saída padrão.

    DUP = 0x08,    ///< Duplica o valor no topo da pilha.
    SWAP = 0x09,   ///< Troca os dois valores no topo da pilha.
    PUSH16 = 0x0A, ///< Empilha um valor de 16 bits: `PUSH16 <uint16_t>`.
    JMP = 0x0B,    ///< Salto incondicional para endereço: `JMP <uint16_t addr>`.
    JZ = 0x0C      ///< Salto condicional se zero: `JZ <uint16_t addr>`.
};

/**
 * @brief Lista constante de opcodes válidos para validação rápida.
 *
 * @note O uso de `constexpr std::array` permite que essa lista seja construída em tempo de compilação,
 * otimizando a verificação de segurança sem custo de tempo de execução para inicialização.
 */
constexpr std::array<Byte, 13> kValidOpcodes = {
    static_cast<Byte>(Opcode::HALT),
    static_cast<Byte>(Opcode::PUSH),
    static_cast<Byte>(Opcode::POP),
    static_cast<Byte>(Opcode::ADD),
    static_cast<Byte>(Opcode::SUB),
    static_cast<Byte>(Opcode::MUL),
    static_cast<Byte>(Opcode::DIV),
    static_cast<Byte>(Opcode::PRINT),
    static_cast<Byte>(Opcode::DUP),
    static_cast<Byte>(Opcode::SWAP),
    static_cast<Byte>(Opcode::PUSH16),
    static_cast<Byte>(Opcode::JMP),
    static_cast<Byte>(Opcode::JZ)
};

/**
 * @brief Verifica se um byte corresponde a um Opcode válido.
 *
 * @param b O byte a ser verificado.
 * @return `true` se o byte for um opcode válido, `false` caso contrário.
 *
 * @note Destaque C++23:
 * - `[[nodiscard]]`: O compilador emitirá um aviso se o retorno desta função for ignorado.
 * - `constexpr`: Permite que esta verificação seja feita em tempo de compilação se o argumento for constante.
 * - `noexcept`: Garante que esta função não lança exceções.
 */
[[nodiscard]] constexpr bool is_valid_opcode(Byte b) noexcept {
    for (Byte v : kValidOpcodes) if (v == b) return true;
    return false;
}

// =========================== VirtualMachine Class ===========================

/**
 * @class VirtualMachine
 * @brief Representa a máquina virtual, contendo memória, pilha e lógica de execução.
 */
class VirtualMachine {
public:
    /**
     * @struct Config
     * @brief Estrutura de configuração da VM.
     */
    struct Config {
        Endianness endianness = Endianness::Big; ///< Define a ordem dos bytes para leitura de palavras (16 bits).
        bool debug = false;                      ///< Se true, imprime o estado da pilha e IP a cada instrução.
    };

    /**
     * @brief Construtor da Máquina Virtual.
     *
     * Inicializa a memória com o programa fornecido e define o Instruction Pointer (IP) como 0.
     *
     * @param program Vetor de bytes contendo o bytecode a ser executado.
     * @param cfg Configurações da VM (opcional).
     */
    explicit VirtualMachine(std::vector<Byte> program, Config cfg = {})
        : memory_{ std::move(program) }, cfg_{ cfg }, ip_{ 0 }, running_{ true }
    {
        // nada extra
    }

    /**
     * @brief Executa o ciclo principal da VM (Fetch-Decode-Execute).
     *
     * O loop continua até que:
     * 1. A instrução `HALT` seja encontrada.
     * 2. Uma exceção `VMError` seja lançada (erro de execução).
     * 3. O IP ultrapasse o tamanho da memória.
     *
     * @throws VMError Se ocorrer um erro fatal durante a execução.
     */
    void run() {
        try {
            while (running_) {
                if (ip_ >= memory_.size()) {
                    throw VMError("IP fora dos limites da memória: " + std::to_string(ip_));
                }

                const Byte raw = fetchByteNoAdvance(); // peek para validar antes de consumir
                if (!is_valid_opcode(raw)) {
                    std::ostringstream oss;
                    oss << "Opcode inválido lido em IP=" << ip_ << " : 0x"
                        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(raw);
                    throw VMError(oss.str());
                }

                const Opcode op = static_cast<Opcode>(fetchByte()); // agora consome
                execute(op);

                if (cfg_.debug) debugDumpState(op);
            }
        }
        catch (...) {
            // relança a exceção para o chamador após imprimir contexto
            std::cerr << "[VM] Exceção em execução. estado final: ip=" << ip_
                << " stack_size=" << stack_.size() << std::endl;
            throw;
        }
    }

private:
    std::vector<Byte> memory_; ///< Memória de programa (Bytecode).
    std::vector<Int> stack_;   ///< Pilha de operandos.
    Config cfg_;               ///< Configurações da instância.
    Address ip_;               ///< Instruction Pointer (Apontador de Instrução).
    bool running_;             ///< Flag de controle do loop principal.

    // =================== Helpers de fetch ===================

    /**
     * @brief Lê o byte atual apontado pelo IP sem avançar o ponteiro.
     * @return O byte lido.
     * @throws VMError Se o IP estiver fora dos limites.
     * @note `[[nodiscard]]` incentiva o uso do valor retornado.
     */
    [[nodiscard]] Byte fetchByteNoAdvance() const {
        if (ip_ >= memory_.size()) throw VMError("fetchByteNoAdvance: IP fora do intervalo");
        return memory_[ip_];
    }

    /**
     * @brief Lê o byte atual e avança o IP.
     * @return O byte lido.
     * @throws VMError Se o IP estiver fora dos limites.
     */
    [[nodiscard]] Byte fetchByte() {
        if (ip_ >= memory_.size()) throw VMError("fetchByte: IP fora do intervalo");
        return memory_[ip_++];
    }

    /**
     * @brief Lê uma palavra de 16 bits (2 bytes) e avança o IP em 2 posições.
     *
     * Respeita a configuração de *Endianness* definida em `cfg_`.
     *
     * @return A palavra de 16 bits combinada.
     * @throws VMError Se houver menos de 2 bytes disponíveis.
     */
    [[nodiscard]] Word16 fetchWord() {
        // Le 2 bytes de memory_ respeitando endianness da config
        if (ip_ + 1 >= memory_.size()) throw VMError("fetchWord: leitura fora dos limites");
        Byte b0 = memory_[ip_];
        Byte b1 = memory_[ip_ + 1];
        ip_ += 2;
        if (cfg_.endianness == Endianness::Big) {
            return static_cast<Word16>((static_cast<Word16>(b0) << 8) | static_cast<Word16>(b1));
        }
        else {
            return static_cast<Word16>((static_cast<Word16>(b1) << 8) | static_cast<Word16>(b0));
        }
    }

    // =================== Stack checks & ops ===================

    /**
     * @brief Garante que a pilha tenha um número mínimo de elementos.
     * @param n Quantidade necessária de elementos.
     * @param opName Nome da operação que solicitou a verificação (para log).
     * @throws VMError Se a pilha não tiver elementos suficientes (Stack Underflow).
     */
    void ensureStackHas(std::size_t n, std::string_view opName) const {
        if (stack_.size() < n) {
            std::ostringstream oss;
            oss << "Pilha insuficiente para " << opName
                << " (necessário: " << n << ", disponível: " << stack_.size() << ")";
            throw VMError(oss.str());
        }
    }

    /**
     * @brief Executa uma operação binária genérica.
     *
     * Desempilha dois valores ($b$, depois $a$), aplica a função $f(a, b)$ e 
     * empilha o resultado.
     *
     * @param func Lambda ou função que aceita dois `Int` e retorna um `Int`.
     * @param name Nome da operação (para mensagens de erro).
     */
    void binaryOp(const std::function<Int(Int, Int)>& func, std::string_view name) {
        ensureStackHas(2, name);
        Int b = stack_.back(); stack_.pop_back();
        Int a = stack_.back(); stack_.pop_back();
        Int r = func(a, b);
        stack_.push_back(r);
    }



    // =================== Execução do opcode ===================

    /**
     * @brief Despacha e executa uma única instrução baseada no Opcode.
     * @param op O Opcode a ser executado.
     */
    void execute(Opcode op) {
        switch (op) {
        case Opcode::HALT:
            running_ = false;
            return;

        case Opcode::PUSH: {
            Byte v = fetchByte();
            stack_.push_back(static_cast<Int>(v));
            return;
        }

        case Opcode::POP:
            ensureStackHas(1, "POP");
            stack_.pop_back();
            return;

        case Opcode::ADD:
            binaryOp([](Int a, Int b) { return a + b; }, "ADD");
            return;

        case Opcode::SUB:
            binaryOp([](Int a, Int b) { return a - b; }, "SUB");
            return;

        case Opcode::MUL:
            binaryOp([](Int a, Int b) { return a * b; }, "MUL");
            return;

        case Opcode::DIV:
            binaryOp([](Int a, Int b) {
                if (b == 0) throw VMError("Divisão por zero");
                return a / b;
                }, "DIV");
            return;

        case Opcode::PRINT:
            ensureStackHas(1, "PRINT");
            std::cout << stack_.back() << '\n';
            stack_.pop_back();
            return;

            // --- Extensões implementadas ---
        case Opcode::DUP:
            ensureStackHas(1, "DUP");
            stack_.push_back(stack_.back());
            return;

        case Opcode::SWAP: {
            ensureStackHas(2, "SWAP");
            std::swap(stack_[stack_.size() - 1], stack_[stack_.size() - 2]);
            return;
        }

        case Opcode::PUSH16: {
            Word16 w = fetchWord();
            stack_.push_back(static_cast<Int>(w));
            return;
        }

        case Opcode::JMP: {
            Word16 addr = fetchWord();
            if (addr >= memory_.size()) {
                throw VMError("JMP: endereço inválido: " + std::to_string(addr));
            }
            ip_ = static_cast<Address>(addr);
            return;
        }

        case Opcode::JZ: {
            Word16 addr = fetchWord();
            ensureStackHas(1, "JZ");
            Int value = stack_.back(); stack_.pop_back();
            if (value == 0) {
                if (addr >= memory_.size()) throw VMError("JZ: endereço inválido: " + std::to_string(addr));
                ip_ = static_cast<Address>(addr);
            }
            return;
        }

        default:
            throw VMError("Opcode desconhecido em execute()");
        }
    }

    // =================== Debug / inspeção ===================

    /**
     * @brief Imprime o estado atual da VM (IP e Pilha) para depuração.
     * @param lastOp A última operação executada.
     */
    void debugDumpState(Opcode lastOp) const {
        std::cout << "[VM DEBUG] IP=" << ip_
            << " lastOp=0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(static_cast<Byte>(lastOp)) << std::dec
            << " stack_size=" << stack_.size()
            << " stack=[";
        for (std::size_t i = 0; i < stack_.size(); ++i) {
            if (i) std::cout << ", ";
            std::cout << stack_[i];
        }
        std::cout << "]\n";
    }
};

// =========================== Helpers para montagem de bytecode ===========================

/**
 * @namespace assembler
 * @brief Funções auxiliares para facilitar a construção manual de programas (bytecode).
 *
 * Fornece uma abstração simples sobre o `std::vector<Byte>` para evitar inserções manuais propensas a erro.
 */
namespace assembler {
    /**
     * @brief Insere um opcode no vetor de bytecode.
     */
    inline void emit(std::vector<Byte>& out, Opcode op) {
        out.push_back(static_cast<Byte>(op));
    }

    /**
     * @brief Emite a instrução `PUSH` seguida de um valor de 8 bits.
     */
    inline void emit_push(std::vector<Byte>& out, Byte value) {
        emit(out, Opcode::PUSH);
        out.push_back(value);
    }

    /**
     * @brief Emite a instrução `PUSH16` seguida de um valor de 16 bits (Big-Endian).
     */
    inline void emit_push16_be(std::vector<Byte>& out, Word16 value) {
        emit(out, Opcode::PUSH16);
        out.push_back(static_cast<Byte>((value >> 8) & 0xFF));
        out.push_back(static_cast<Byte>(value & 0xFF));
    }

    /**
     * @brief Emite a instrução `JMP` (salto incondicional) com endereço Big-Endian.
     */
    inline void emit_jmp_be(std::vector<Byte>& out, Word16 addr) {
        emit(out, Opcode::JMP);
        out.push_back(static_cast<Byte>((addr >> 8) & 0xFF));
        out.push_back(static_cast<Byte>(addr & 0xFF));
    }

    /**
     * @brief Emite a instrução `JZ` (salto se zero) com endereço Big-Endian.
     */
    inline void emit_jz_be(std::vector<Byte>& out, Word16 addr) {
        emit(out, Opcode::JZ);
        out.push_back(static_cast<Byte>((addr >> 8) & 0xFF));
        out.push_back(static_cast<Byte>(addr & 0xFF));
    }
}

// =========================== Programas de Teste ===========================

/**
 * @brief Cria um programa de teste aritmético simples.
 *
 * Lógica: $(10 + 5) \times 2 = 30$.
 *
 * @return Bytecode do programa.
 */
static std::vector<Byte> make_program1() {
    // Programa 1: (10 + 5) * 2 -> imprime 30
    // PUSH 10; PUSH 5; ADD; PUSH 2; MUL; PRINT; HALT
    std::vector<Byte> p;
    assembler::emit_push(p, 10);
    assembler::emit_push(p, 5);
    assembler::emit(p, Opcode::ADD);
    assembler::emit_push(p, 2);
    assembler::emit(p, Opcode::MUL);
    assembler::emit(p, Opcode::PRINT);
    assembler::emit(p, Opcode::HALT);
    return p;
}

/**
 * @brief Cria um programa de teste com loops e condicionais.
 *
 * Lógica: Contagem regressiva de 3 até 1.
 * Utiliza manipulação de pilha (`DUP`) e saltos (`JZ`, `JMP`).
 *
 * @return Bytecode do programa.
 */
static std::vector<Byte> make_program2_countdown() {
    // Programa 2: contagem regressiva de 3 a 1 usando DUP, JZ, JMP
    // Endereços calculados manualmente para clareza (big-endian)
    // layout:
    // 00: PUSH 3
    // 02: DUP
    // 03: PRINT
    // 04: PUSH 1
    // 06: SUB
    // 07: DUP
    // 08: JZ 0x0014
    // 11: JMP 0x0002
    // 14: POP
    // 15: HALT

    std::vector<Byte> p;
    assembler::emit_push(p, 3);           // 00,01

    assembler::emit(p, Opcode::DUP);     // 02
    assembler::emit(p, Opcode::PRINT);   // 03
    assembler::emit_push(p, 1);          // 04,05
    assembler::emit(p, Opcode::SUB);     // 06
    assembler::emit(p, Opcode::DUP);     // 07

    // JZ -> endereço 0x0014 (decimal 20). colocamos big-endian 00 14
    assembler::emit_jz_be(p, static_cast<Word16>(0x0014)); // 08,09,10

    // JMP -> endereço 0x0002
    assembler::emit_jmp_be(p, static_cast<Word16>(0x0002)); // 11,12,13

    assembler::emit(p, Opcode::POP);     // 14
    assembler::emit(p, Opcode::HALT);    // 15

    return p;
}

// =========================== Main: execuções de teste ===========================

/**
 * @brief Ponto de entrada principal.
 *
 * Executa os dois programas de exemplo sequencialmente, capturando exceções de execução.
 *
 * @return 0 em caso de sucesso, códigos de erro > 0 em caso de falha.
 */
int main() {
    try {
        std::cout << "--- Iniciando VM (Programa 1) ---\n";
        {
            auto program1 = make_program1();
            VirtualMachine::Config cfg;
            cfg.endianness = Endianness::Big; // corresponde ao assembler emitido
            cfg.debug = false; // set true para acompanhar cada instrução
            VirtualMachine vm(std::move(program1), cfg);
            vm.run();
        }
        std::cout << "--- VM Finalizada (Programa 1) ---\n\n";

        std::cout << "--- Iniciando VM (Programa 2 - contagem regressiva) ---\n";
        {
            auto program2 = make_program2_countdown();
            VirtualMachine::Config cfg;
            cfg.endianness = Endianness::Big;
            cfg.debug = false;
            VirtualMachine vm(std::move(program2), cfg);
            vm.run();
        }
        std::cout << "--- VM Finalizada (Programa 2) ---\n";
    }
    catch (const VMError& e) {
        std::cerr << "Erro na VM: " << e.what() << std::endl;
        return 2;
    }
    catch (const std::exception& e) {
        std::cerr << "Erro inesperado: " << e.what() << std::endl;
        return 3;
    }
    return 0;
}