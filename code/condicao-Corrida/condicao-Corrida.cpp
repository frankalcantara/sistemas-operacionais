#include <iostream>
#include <thread>
#include <vector>

long long contador = 0;

void incrementa() {
    for (int i = 0; i < 100000; ++i) {
        // Opera��o n�o at�mica!
        // Leitura, modifica��o e escrita s�o 3 passos.
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

    // O resultado ser� quase sempre MENOR que 1.000.000
    std::cout << "Resultado final: " << contador << std::endl;
    return 0;
}