#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <functional>

class VirtualMachine {
public:
    explicit VirtualMachine(const std::vector<uint8_t>& program)
        : memory(program), ip(0) {
    }

    void run() {
        while (ip < memory.size()) {
            Opcode op = static_cast<Opcode>(memory[ip++]);
            execute(op);
        }
    }

private:
    std::vector<uint8_t> memory;
    std::vector<int> stack;
    size_t ip;

    void execute(Opcode op) {
        switch (op) {
        case Opcode::HALT:
            ip = memory.size(); // força saída do loop
            break;

        case Opcode::PUSH: {
            if (ip >= memory.size())
                throw std::runtime_error("Falta operando para PUSH");
            uint8_t value = memory[ip++];
            stack.push_back(static_cast<int>(value));
            break;
        }

        case Opcode::POP:
            checkStack(1, "POP");
            stack.pop_back();
            break;

        case Opcode::ADD: binaryOp(<a href = "int a, int b" target = "_blank" rel = "noopener noreferrer nofollow">< / a> { return a + b; }, "ADD"); break;
        case Opcode::SUB: binaryOp(<a href = "int a, int b" target = "_blank" rel = "noopener noreferrer nofollow">< / a> { return a - b; }, "SUB"); break;
        case Opcode::MUL: binaryOp(<a href = "int a, int b" target = "_blank" rel = "noopener noreferrer nofollow">< / a> { return a * b; }, "MUL"); break;
        case Opcode::DIV: binaryOp(<a href = "int a, int b" target = "_blank" rel = "noopener noreferrer nofollow">< / a> {
            if (b == 0) throw std::runtime_error("Divisão por zero");
            return a / b;
        }, "DIV"); break;

        case Opcode::PRINT:
            checkStack(1, "PRINT");
            std::cout << "Output: " << stack.back() << std::endl;
            stack.pop_back();
            break;

        default:
            throw std::runtime_error("Opcode desconhecido: " + std::to_string(static_cast<uint8_t>(op)));
        }
    }

    void checkStack(size_t n, const std::string& op) const {
        if (stack.size() < n)
            throw std::runtime_error("Pilha insuficiente para " + op + " (necessário: " + std::to_string(n) + ")");
    }

    void binaryOp(std::function<int(int, int)> func, const std::string& name) {
        checkStack(2, name);
        int b = stack.back(); stack.pop_back();
        int a = stack.back(); stack.pop_back();
        stack.push_back(func(a, b));
    }
};