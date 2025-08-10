#include <iostream>

int main() {
    // ENTRADA
    double idade, rendaMensal, patrimonioLiquido, experienciaInvestimentos, toleranciaPerda;
    std::string objetivoPrazo;

    std::cout << "Idade: ";
    std::cin >> idade;

    std::cout << "Renda mensal (R$): ";
    std::cin >> rendaMensal;

    std::cout << "Patrimônio líquido (R$): ";
    std::cin >> patrimonioLiquido;

    std::cout << "Experiência em investimentos (anos): ";
    std::cin >> experienciaInvestimentos;

    std::cout << "Tolerância à perda (%): ";
    std::cin >> toleranciaPerda;

    std::cout << "Objetivo de prazo (curto/medio/longo): ";
    std::cin >> objetivoPrazo;

    // PROCESSAMENTO
    int pontuacaoTotal = 0;

    if (idade < 30) {
        pontuacaoTotal = pontuacaoTotal + 2;
    }
    else if (idade <= 50) {
        pontuacaoTotal = pontuacaoTotal + 1;
    }

    if (rendaMensal > 10000) {
        pontuacaoTotal = pontuacaoTotal + 2;
    }
    else if (rendaMensal >= 5000) {
        pontuacaoTotal = pontuacaoTotal + 1;
    }

    if (patrimonioLiquido > 100000) {
        pontuacaoTotal = pontuacaoTotal + 2;
    }
    else if (patrimonioLiquido >= 50000) {
        pontuacaoTotal = pontuacaoTotal + 1;
    }

    if (experienciaInvestimentos > 5) {
        pontuacaoTotal = pontuacaoTotal + 2;
    }
    else if (experienciaInvestimentos >= 2) {
        pontuacaoTotal = pontuacaoTotal + 1;
    }

    if (toleranciaPerda > 20) {
        pontuacaoTotal = pontuacaoTotal + 2;
    }
    else if (toleranciaPerda >= 10) {
        pontuacaoTotal = pontuacaoTotal + 1;
    }

    if (objetivoPrazo == "longo") {
        pontuacaoTotal = pontuacaoTotal + 2;
    }
    else if (objetivoPrazo == "medio") {
        pontuacaoTotal = pontuacaoTotal + 1;
    }

    std::string perfilInvestidor;

    if (pontuacaoTotal <= 4) {
        perfilInvestidor = "Conservador";
    }
    else if (pontuacaoTotal <= 8) {
        perfilInvestidor = "Moderado";
    }
    else {
        perfilInvestidor = "Arrojado";
    }

    if ((idade > 60) && (toleranciaPerda < 15)) {
        perfilInvestidor = "Conservador";
    }

    // SAÍDA
    std::cout << "Pontuação total: " << pontuacaoTotal << std::endl;
    std::cout << "Perfil do investidor: " << perfilInvestidor << std::endl;

    return 0;
}