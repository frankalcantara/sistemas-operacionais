/**
 * @file shared_struct.h
 * @brief Definição da estrutura de dados compartilhada e constantes.
 *
 * Este arquivo pode ser incluído tanto pelo leitor quanto pelo escritor
 * para garantir que ambos usem as mesmas definições.
 */

#pragma once // Evita inclusão múltipla do cabeçalho

 /// @brief Tamanho máximo da mensagem no buffer, em caracteres.
const int MAX_MESSAGE_SIZE = 256;

/**
 * @struct SharedData
 * @brief Estrutura que será alocada na memória compartilhada.
 *
 * Contém apenas os dados da aplicação, já que a sincronização
 * será feita por objetos nomeados do kernel do Windows.
 */
struct SharedData {
    /// @brief Buffer para armazenar a mensagem (usando wchar_t para compatibilidade com a API Unicode do Windows).
    wchar_t message[MAX_MESSAGE_SIZE];

    /// @brief Flag para sinalizar ao leitor que a comunicação deve terminar.
    bool exit_requested;
};

// --- Nomes dos Objetos do Kernel ---

/// @brief Nome para o objeto de mapeamento de arquivo (memória compartilhada).
const wchar_t* SHM_NAME = L"MySharedMemoryObject";

/// @brief Nome para o Mutex que protegerá o acesso à memória.
const wchar_t* MUTEX_NAME = L"MySharedMemoryMutex";

/// @brief Nome do Evento que sinaliza que o buffer está cheio (mensagem pronta para o leitor).
const wchar_t* EVENT_FULL_NAME = L"MySharedMemoryEventFull";

/// @brief Nome do Evento que sinaliza que o buffer está vazio (pronto para o escritor).
const wchar_t* EVENT_EMPTY_NAME = L"MySharedMemoryEventEmpty";
