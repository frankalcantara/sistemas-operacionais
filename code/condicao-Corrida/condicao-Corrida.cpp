#include <iostream>
#include <thread>
#include <vector>

long long contador = 0;

void incrementa() {
    for (int i = 0; i < 100000; ++i) {
        // Operação não atômica!
        // Leitura, modificação e escrita são 3 passos.
        contador++;
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.push_back(std::thread(incrementa));
    }

    for (auto& t : threads) {
        t.join();
    }

    // O resultado será quase sempre MENOR que 1.000.000
    std::cout << "Resultado final: " << contador << std::endl;
    return 0;
}