/**
 * @file reader.cpp
 * @brief Programa leitor usando a API nativa do Windows.
 *
 * Abre um segmento de memória compartilhada existente e os objetos de
 * sincronização para ler mensagens enviadas pelo programa escritor.
 */

#include <windows.h>
#include <iostream>
#include "shared_struct.h" // Inclui a definição da struct e constantes

void PrintError(const std::string& functionName) {
    DWORD error = GetLastError();
    std::cerr << "Erro em " << functionName << ": " << error << std::endl;
}

/**
 * @brief Função principal do programa leitor.
 * @return int Retorna 0 em caso de sucesso, 1 em caso de erro.
 */
int main() {
    // --- 1. Abertura do Mapeamento de Arquivo Existente ---
    HANDLE hMapFile = OpenFileMappingW(
        FILE_MAP_READ,           // Acesso de leitura
        FALSE,                   // Não herdar handle
        SHM_NAME                 // Nome do mapeamento
    );

    if (hMapFile == NULL) {
        PrintError("OpenFileMappingW. O escritor está em execução?");
        return 1;
    }

    // --- 2. Mapeamento da Memória ---
    SharedData* pSharedData = (SharedData*)MapViewOfFile(
        hMapFile,
        FILE_MAP_READ,           // Acesso de leitura
        0,
        0,
        sizeof(SharedData)
    );

    if (pSharedData == nullptr) {
        PrintError("MapViewOfFile");
        CloseHandle(hMapFile);
        return 1;
    }

    // --- 3. Abertura dos Objetos de Sincronização Existentes ---
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, MUTEX_NAME);
    HANDLE hEventFull = OpenEventW(SYNCHRONIZE, FALSE, EVENT_FULL_NAME);
    HANDLE hEventEmpty = OpenEventW(EVENT_MODIFY_STATE, FALSE, EVENT_EMPTY_NAME);

    if (hMutex == NULL || hEventFull == NULL || hEventEmpty == NULL) {
        PrintError("OpenMutexW/OpenEventW");
        UnmapViewOfFile(pSharedData);
        CloseHandle(hMapFile);
        return 1;
    }

    std::wcout << L"Programa leitor iniciado. Aguardando mensagens..." << std::endl;

    // Loop principal para ler mensagens
    while (true) {
        // Espera pelo sinal de que o buffer está cheio (o escritor enviou algo)
        WaitForSingleObject(hEventFull, INFINITE);

        // Solicita posse do mutex
        WaitForSingleObject(hMutex, INFINITE);

        // --- Seção Crítica ---
        if (pSharedData->exit_requested) {
            ReleaseMutex(hMutex); // Libera o mutex antes de sair
            break;
        }

        std::wcout << L"Mensagem recebida: " << pSharedData->message << std::endl;
        // --- Fim da Seção Crítica ---

        // Libera o mutex
        ReleaseMutex(hMutex);

        // Sinaliza o evento "vazio", acordando o escritor
        SetEvent(hEventEmpty);
    }

    std::wcout << L"Sinal de encerramento recebido. Encerrando o leitor." << std::endl;

    // --- 4. Limpeza ---
    UnmapViewOfFile(pSharedData);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    CloseHandle(hEventFull);
    CloseHandle(hEventEmpty);

    return 0;
}