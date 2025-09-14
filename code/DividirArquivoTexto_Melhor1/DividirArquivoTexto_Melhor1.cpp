/**
 * @file main.cpp
 * @brief Programa principal para baixar texto de uma URL, dividi-lo em partes lógicas e salvar em arquivos.
 *
 * Este programa é um exemplo simples de como usar APIs do Windows para download de conteúdo web,
 * processar texto em streaming (sem carregar tudo na memória de uma vez) e salvar partes em arquivos.
 * Ele é projetado para rodar no Windows, usando o Visual Studio ou similar.
 *
 * Compilação: No Visual Studio, certifique-se de que o projeto está configurado para C++23 ou superior,
 * e que a biblioteca wininet.lib está linkada (o #pragma faz isso automaticamente no MSVC).
 * Além disso, você precisará configurar os comandos url, partes e nome do diretório direto nas opções do
 * projeto, ou passar como argumentos na linha de comando.
 */

 // Necessário para entrada/saída no console, como std::cout e std::cerr. É a base para imprimir mensagens.
#include <iostream>

// Para manipular strings (sequências de caracteres), como std::string e std::wstring.
#include <string>

// Para vetores dinâmicos (arrays que crescem), embora não usado diretamente aqui, pode ser útil para extensões.
#include <vector>

// Para lançar exceções em erros, como std::runtime_error.
#include <stdexcept>

// Para trabalhar com arquivos de saída, como std::ofstream.
#include <fstream> // Para std::ofstream (escrita em arquivos)

// Para manipular diretórios e arquivos no sistema de arquivos, como criar pastas.
#include <filesystem> // Para manipulação de diretórios

// Para formatar saídas, como adicionar zeros à esquerda em números (ex: 001).
#include <iomanip> // Para std::setw e std::setfill (formatar nomes de arquivos)

// Para construir strings dinamicamente, como nomes de arquivos.
#include <sstream> // Para construir os nomes dos arquivos dinamicamente

// Para algoritmos úteis, como std::find para procurar caracteres em strings.
#include <algorithm> // Para std::find

// Para funções de strings C, como std::strlen para medir comprimento de char*.
#include <cstring> // Para strlen em main

// Cabeçalhos do Windows SDK para funcionalidades específicas do Windows.
// Necessário para funções de sistema, como conversões de encoding e rede.
#include <windows.h>

// Para download de URLs via HTTP, parte do Windows Internet API.
#include <wininet.h>

// Diretiva para o linker incluir automaticamente a biblioteca wininet.lib no MSVC.
// Isso evita configurar manualmente no projeto do Visual Studio.
#pragma comment(lib, "wininet.lib")

/**
 * @brief Converte uma string em UTF-8 para uma wide string (wstring) usando APIs do Windows.
 *
 * @param utf8 A string de entrada no formato UTF-8 (comum em textos web).
 * @return std::wstring A versão wide (Unicode) da string.
 * @throw std::runtime_error Se a conversão falhar.
 *
 * Para alunos iniciantes: UTF-8 é um jeito de armazenar letras e símbolos (como acentos ou emojis) em bytes.
 * Wide strings (wstring) usam mais espaço para guardar esses símbolos de forma segura no Windows.
 * Esta função usa ferramentas do Windows (MultiByteToWideChar) para transformar um formato no outro,
 * garantindo que caracteres especiais não sejam corrompidos.
 */
std::wstring Utf8ToWstring(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len == 0) throw std::runtime_error("Falha na conversao UTF-8 para wstring.");
    std::wstring wstr(len - 1, 0); // len inclui null terminator
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], len);
    return wstr;
}

/**
 * @brief Converte uma wide string (wstring) de volta para uma string em UTF-8 usando APIs do Windows.
 *
 * @param wstr A wide string de entrada (Unicode).
 * @return std::string A versão UTF-8 da string.
 * @throw std::runtime_error Se a conversão falhar.
 *
 * Para alunos iniciantes: Esta é o oposto da função anterior. Depois de processar o texto em wide strings
 * (mais seguras para manipulação), convertemos de volta para UTF-8 para salvar no arquivo, pois UTF-8
 * é compacto e compatível com a maioria dos editores de texto.
 */
std::string WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) throw std::runtime_error("Falha na conversao wstring para UTF-8.");
    std::string utf8(len - 1, 0); // len inclui null terminator
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}

/**
 * @brief Baixa o conteúdo de uma URL, divide em partes lógicas e salva em arquivos, tudo em streaming.
 *
 * @param url A URL do texto a ser baixado (deve ser wide string para compatibilidade com WinINet).
 * @param numeroDePartes O número de partes em que dividir o texto.
 * @param nome_diretorio O nome da pasta onde salvar os arquivos.
 * @throw std::runtime_error Em falhas de rede ou conversão.
 *
 * Para alunos iniciantes: Esta é a função principal do programa. Ela:
 * 1. Conecta à internet usando ferramentas do Windows (WinINet) para baixar o texto da URL.
 * 2. Lê o texto em pedaços pequenos (chunks de 4096 bytes) para não usar muita memória.
 * 3. Converte cada pedaço para wide string para manipular com segurança (evitar cortes em caracteres especiais).
 * 4. Acumula pedaços até atingir um tamanho aproximado por parte, então divide no próximo '\n' (nova linha) para não cortar frases no meio.
 * 5. Salva cada parte em um arquivo separado (ex: parte_001.txt) na pasta especificada.
 * 6. No final, salva o que sobrou como a última parte.
 *
 * Isso é eficiente para textos grandes, como livros, porque não carrega tudo na RAM de uma vez.
 */
void baixarEDividirESalvar(const std::wstring& url, int numeroDePartes, const std::string& nome_diretorio) {
    HINTERNET hInternet = nullptr;
    HINTERNET hConnect = nullptr;

    hInternet = InternetOpenW(L"CppHttpClient", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        throw std::runtime_error("Falha ao chamar InternetOpen.");
    }
    hConnect = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        throw std::runtime_error("Falha ao chamar InternetOpenUrl.");
    }

    // Cria o diretório
    std::cout << "Criando diretorio de saida: " << nome_diretorio << std::endl;
    std::filesystem::create_directory(nome_diretorio);

    char buffer[4096];
    DWORD bytesRead = 0;
    std::wstring accumulated; // Buffer temporário para chunks (não o texto todo)
    size_t totalLength = 0; // Estimar comprimento total se possível, mas WinINet não fornece facilmente; assumimos divisão aproximada
    DWORD contentLength = 0;
    DWORD size = sizeof(contentLength);
    if (HttpQueryInfo(hConnect, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &size, NULL)) {
        totalLength = contentLength; // Usar para calcular tamanho aproximado da parte (em bytes, aproximado para chars)
    }
    else {
        std::cerr << "Não foi possível obter o tamanho total do conteúdo. Usando divisão aproximada." << std::endl;
    }
    size_t tamanhoDaParte = (totalLength > 0) ? totalLength / numeroDePartes : 4096 * 100; // Fallback arbitrário (em bytes, aproximado)

    int parteAtual = 0;
    std::ofstream arquivo_saida;
    std::wstring currentPart;

    std::cout << "Baixando e processando o texto de: " << WstringToUtf8(url) << std::endl;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        // Converter chunk para wstring (UTF-8 safe)
        std::wstring chunk = Utf8ToWstring(std::string(buffer, bytesRead));
        accumulated += chunk;

        // Enquanto o accumulated for maior que tamanhoDaParte, dividir logicamente
        while (accumulated.length() >= tamanhoDaParte && parteAtual < numeroDePartes - 1) {
            // Encontrar o próximo '\n' após o ponto de corte aproximado
            size_t posCorte = tamanhoDaParte;
            auto it = std::find(accumulated.begin() + posCorte, accumulated.end(), L'\n');
            if (it != accumulated.end()) {
                posCorte = std::distance(accumulated.begin(), it) + 1; // Incluir o '\n'
            }
            else {
                // Se não encontrar '\n', corta no fim do buffer
                posCorte = accumulated.length();
            }

            currentPart = accumulated.substr(0, posCorte);
            accumulated = accumulated.substr(posCorte);

            // Salvar a parte
            std::stringstream ss;
            ss << nome_diretorio << "/parte_"
                << std::setw(3) << std::setfill('0') << (parteAtual + 1)
                << ".txt";
            std::string nome_arquivo = ss.str();

            arquivo_saida.open(nome_arquivo, std::ios::binary); // Binary para preservar UTF-8
            if (!arquivo_saida.is_open()) {
                std::cerr << "Erro ao criar o arquivo: " << nome_arquivo << std::endl;
                continue;
            }
            // Converter de volta para UTF-8 e escrever
            arquivo_saida << WstringToUtf8(currentPart);
            arquivo_saida.close();

            std::cout << "Parte " << (parteAtual + 1) << " salva em " << nome_arquivo << std::endl;
            parteAtual++;
        }
    }

    // Salvar a última parte (restante)
    if (!accumulated.empty() && parteAtual < numeroDePartes) {
        currentPart = accumulated;

        std::stringstream ss;
        ss << nome_diretorio << "/parte_"
            << std::setw(3) << std::setfill('0') << (parteAtual + 1)
            << ".txt";
        std::string nome_arquivo = ss.str();

        arquivo_saida.open(nome_arquivo, std::ios::binary);
        if (!arquivo_saida.is_open()) {
            std::cerr << "Erro ao criar o arquivo: " << nome_arquivo << std::endl;
        }
        else {
            arquivo_saida << WstringToUtf8(currentPart);
            arquivo_saida.close();
            std::cout << "Parte final " << (parteAtual + 1) << " salva em " << nome_arquivo << std::endl;
        }
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    std::cout << (parteAtual + 1) << " partes foram salvas com sucesso no diretorio '" << nome_diretorio << "'." << std::endl;
}

/**
 * @brief Função principal do programa, que processa argumentos de linha de comando e chama a função de download.
 *
 * @param argc Número de argumentos passados (incluindo o nome do executável).
 * @param argv Array de strings com os argumentos.
 * @return int Código de saída: 0 para sucesso, 1 para erro.
 *
 * Para alunos iniciantes: Todo programa C++ começa aqui, no main(). Ele verifica se você passou os argumentos certos
 * quando roda o programa (ex: no prompt de comando: programa.exe https://exemplo.com 10 pasta_saida).
 * Os argumentos são: 1. URL do texto, 2. Número de partes, 3. Nome da pasta.
 * Se faltar, mostra como usar e sai. Senão, converte a URL para wide string e chama a função principal.
 * Usa try-catch para capturar erros e imprimir mensagens amigáveis.
 */
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " <URL> <numero_de_partes> <nome_diretorio>" << std::endl;
        return 1;
    }

    // Converter URL char* para wstring
    std::wstring url;
    url.assign(argv[1], argv[1] + std::strlen(argv[1]));

    int numeroDePartes = std::stoi(argv[2]);
    std::string nome_diretorio = argv[3];

    try {
        baixarEDividirESalvar(url, numeroDePartes, nome_diretorio);
    }
    catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// --- Seção de Testes Unitários Simples (sem bibliotecas externas) ---
// Para testes, compile e execute com argumentos de teste ou descomente abaixo para rodar manualmente.
// Estes são testes básicos; para mais robustez, use asserts manuais.

/**
 * @brief Seção de testes unitários simples para verificar partes do código.
 *
 * Para alunos iniciantes: Testes são como "provas" para o código. Eles verificam se funções funcionam como esperado.
 * Esta seção está desabilitada (#if 0), mas você pode ativar para rodar testes manuais.
 * Ex: No main, chame testeDivisaoLogica(); para ver se passou ou falhou.
 */
#if 0 // Desabilitado; ative para testes

 /**
  * @brief Teste simples para a lógica de divisão do texto.
  *
  * Verifica se a divisão em '\n' funciona corretamente em uma string de exemplo.
  */
void testeDivisaoLogica() {
    std::wstring texto = L"Texto inicial\ncom linhas.\nMais texto\nfinal.";
    size_t tamanhoDaParte = 10;
    size_t posCorte = tamanhoDaParte;
    auto it = std::find(texto.begin() + posCorte, texto.end(), L'\n');
    if (it != texto.end()) {
        posCorte = std::distance(texto.begin(), it) + 1;
    }
    std::wstring parte1 = texto.substr(0, posCorte);
    std::wstring parte2 = texto.substr(posCorte);

    if (parte1 == L"Texto inicial\ncom linhas.\n" && parte2 == L"Mais texto\nfinal.") {
        std::cout << "Teste de divisao logica: PASSOU" << std::endl;
    }
    else {
        std::cout << "Teste de divisao logica: FALHOU" << std::endl;
    }
}

/**
 * @brief Teste simples para as funções de conversão UTF-8.
 *
 * Verifica se converter para wstring e voltar preserva o texto original, incluindo caracteres especiais.
 */
void testeConversoesUtf8() {
    std::string utf8 = "Olá, mundo! 😊";
    std::wstring wstr = Utf8ToWstring(utf8);
    std::string back = WstringToUtf8(wstr);

    if (utf8 == back) {
        std::cout << "Teste de conversoes UTF-8: PASSOU" << std::endl;
    }
    else {
        std::cout << "Teste de conversoes UTF-8: FALHOU" << std::endl;
    }
}

// Chame no main para rodar testes: testeDivisaoLogica(); testeConversoesUtf8();

#endif