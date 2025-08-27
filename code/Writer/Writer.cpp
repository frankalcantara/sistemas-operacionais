/**
 * @file writer.cpp
 * @brief Programa escritor usando a API nativa do Windows.
 *
 * Cria um segmento de memória compartilhada, um mutex e dois eventos para
 * enviar mensagens de forma sincronizada para o programa leitor.
 */

#include <windows.h>
#include <iostream>
#include <string>
#include "shared_struct.h" // Inclui a definição da struct e constantes

 // Função auxiliar para imprimir erros da API do Windows
void PrintError(const std::string& functionName) {
    DWORD error = GetLastError();
    std::cerr << "Erro em " << functionName << ": " << error << std::endl;
}

/**
 * @brief Função principal do programa escritor.
 * @return int Retorna 0 em caso de sucesso, 1 em caso de erro.
 */
int main() {
    // --- 1. Criação do Mapeamento de Arquivo 
    // (Memória Compartilhada) ---
	// Os métodos estão no Windows.h
    HANDLE hMapFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE, // Usar o arquivo de paginação do sistema
        NULL, // Atributos de segurança padrão
        PAGE_READWRITE, // Acesso de leitura/escrita
        0, // Tamanho máximo (high-order DWORD)
        sizeof(SharedData), // Tamanho mínimo (low-order DWORD)
        SHM_NAME // Nome do mapeamento
    );

    if (hMapFile == NULL) {
        PrintError("CreateFileMappingW");
        return 1;
    }

    // --- 2. Mapeamento da Memória para o Espaço de Endereçamento do Processo ---
    SharedData* pSharedData = (SharedData*)MapViewOfFile(
        hMapFile,                // Handle para o mapeamento de arquivo
        FILE_MAP_ALL_ACCESS,     // Permissão de acesso: leitura/escrita
        0,
        0,
        sizeof(SharedData)
    );

    if (pSharedData == nullptr) {
        PrintError("MapViewOfFile");
        CloseHandle(hMapFile);
        return 1;
    }

    // Inicializa os dados na memória compartilhada
    pSharedData->exit_requested = false;
    pSharedData->message[0] = L'\0';

    // --- 3. Criação dos Objetos de Sincronização ---
    HANDLE hMutex = CreateMutexW(NULL, FALSE, MUTEX_NAME);
    HANDLE hEventFull = CreateEventW(NULL, FALSE, FALSE, EVENT_FULL_NAME); // Auto-reset, não sinalizado inicialmente
    HANDLE hEventEmpty = CreateEventW(NULL, FALSE, TRUE, EVENT_EMPTY_NAME); // Auto-reset, sinalizado inicialmente (buffer está vazio)

    if (hMutex == NULL || hEventFull == NULL || hEventEmpty == NULL) {
        PrintError("CreateMutexW/CreateEventW");
        UnmapViewOfFile(pSharedData);
        CloseHandle(hMapFile);
        return 1;
    }

    std::wcout << L"Programa escritor iniciado. Digite mensagens (use 'exit' para sair)." << std::endl;

    // Loop principal para enviar mensagens
    while (true) {
        std::wstring wline;
        std::wcout << L"> ";
        std::getline(std::wcin, wline);

        // Espera pelo sinal de que o buffer está vazio (o leitor terminou de ler)
        WaitForSingleObject(hEventEmpty, INFINITE);

        // Solicita posse do mutex
        WaitForSingleObject(hMutex, INFINITE);

        // --- Seção Crítica ---
        if (wline == L"exit") {
            pSharedData->exit_requested = true;
        }
        else {
            // Copia a mensagem para a memória compartilhada
            wcsncpy_s(pSharedData->message, MAX_MESSAGE_SIZE, wline.c_str(), _TRUNCATE);
        }
        // --- Fim da Seção Crítica ---

        // Libera o mutex
        ReleaseMutex(hMutex);

        // Sinaliza o evento "cheio", acordando o leitor
        SetEvent(hEventFull);

        if (pSharedData->exit_requested) {
            break;
        }
    }

    std::wcout << L"Encerrando o escritor..." << std::endl;

    // --- 4. Limpeza ---
    UnmapViewOfFile(pSharedData);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    CloseHandle(hEventFull);
    CloseHandle(hEventEmpty);

    return 0;
}