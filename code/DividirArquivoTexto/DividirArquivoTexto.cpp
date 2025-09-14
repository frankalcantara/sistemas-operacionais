#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>      // Para std::ofstream (escrita em arquivos)
#include <filesystem>   // Para manipulação de diretórios
#include <iomanip>      // Para std::setw e std::setfill (formatar nomes de arquivos)
#include <sstream>      // Para construir os nomes dos arquivos dinamicamente

// Cabeçalhos do Windows SDK para a funcionalidade de rede
#include <windows.h>
#include <wininet.h>

// Diretiva para o linker incluir a biblioteca wininet.lib
#pragma comment(lib, "wininet.lib")

/**
 * @brief Baixa o conteúdo de uma URL como texto.
 * (Esta função permanece inalterada)
 */
std::string baixarTexto(const std::wstring& url) {
    HINTERNET hInternet = nullptr;
    HINTERNET hConnect = nullptr;
    std::string downloadedData;

    hInternet = InternetOpenW(L"CppHttpClient", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        throw std::runtime_error("Falha ao chamar InternetOpen.");
    }

    hConnect = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        throw std::runtime_error("Falha ao chamar InternetOpenUrl.");
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        downloadedData.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return downloadedData;
}

/**
 * @brief Divide um texto grande em um número específico de partes menores.
 * (Esta função permanece inalterada)
 */
std::vector<std::string> dividirTexto(const std::string& texto, int numeroDePartes) {
    if (texto.empty() || numeroDePartes <= 0) {
        return {};
    }

    std::vector<std::string> partes;
    partes.reserve(numeroDePartes);

    size_t tamanhoTotal = texto.length();
    size_t tamanhoDaParte = tamanhoTotal / numeroDePartes;
    size_t pos_inicial = 0;

    for (int i = 0; i < numeroDePartes; ++i) {
        if (i == numeroDePartes - 1) {
            partes.push_back(texto.substr(pos_inicial));
        }
        else {
            partes.push_back(texto.substr(pos_inicial, tamanhoDaParte));
            pos_inicial += tamanhoDaParte;
        }
    }

    return partes;
}

int main() {
    const std::wstring url = L"https://www.gutenberg.org/files/1342/1342-0.txt"; // Pride and Prejudice
    const int NUMERO_DE_PARTES = 100;
    const std::string nome_diretorio = "textos_divididos"; // Nome da pasta de saída

    try {
        // --- NOVO: Cria o diretório para salvar os arquivos ---
        std::cout << "Criando diretorio de saida: " << nome_diretorio << std::endl;
        std::filesystem::create_directory(nome_diretorio);

        std::cout << "Baixando o texto de: " << std::string(url.begin(), url.end()) << "..." << std::endl;
        std::string textoCompleto = baixarTexto(url);

        std::cout << "Download concluido. Total de " << textoCompleto.length() << " bytes." << std::endl;
        std::cout << "Dividindo o texto em " << NUMERO_DE_PARTES << " partes..." << std::endl;

        std::vector<std::string> textosMenores = dividirTexto(textoCompleto, NUMERO_DE_PARTES);

        std::cout << "Divisao concluida. Salvando arquivos..." << std::endl;

        // --- ALTERADO: Loop para salvar cada parte em um arquivo ---
        for (int i = 0; i < textosMenores.size(); ++i) {
            // Constrói o nome do arquivo com zero à esquerda (ex: parte_001.txt)
            std::stringstream ss;
            ss << nome_diretorio << "/parte_"
                << std::setw(3) << std::setfill('0') << (i + 1)
                << ".txt";
            std::string nome_arquivo = ss.str();

            // Abre o arquivo para escrita
            std::ofstream arquivo_saida(nome_arquivo);
            if (!arquivo_saida.is_open()) {
                std::cerr << "Erro ao criar o arquivo: " << nome_arquivo << std::endl;
                continue; // Pula para a próxima iteração
            }

            // Escreve o conteúdo da parte no arquivo
            arquivo_saida << textosMenores[i];

            // O arquivo é fechado automaticamente quando 'arquivo_saida' sai de escopo
        }

        std::cout << "\n" << textosMenores.size() << " arquivos foram salvos com sucesso no diretorio '" << nome_diretorio << "'." << std::endl;

    }
    catch (const std::exception& e) { // Usando std::exception para pegar também erros do filesystem
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}