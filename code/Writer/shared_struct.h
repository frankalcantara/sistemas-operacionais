/**
 * @file shared_struct.h
 * @brief Defini��o da estrutura de dados compartilhada e constantes.
 *
 * Este arquivo pode ser inclu�do tanto pelo leitor quanto pelo escritor
 * para garantir que ambos usem as mesmas defini��es.
 */

#pragma once // Evita inclus�o m�ltipla do cabe�alho

 /// @brief Tamanho m�ximo da mensagem no buffer, em caracteres.
const int MAX_MESSAGE_SIZE = 256;

/**
 * @struct SharedData
 * @brief Estrutura que ser� alocada na mem�ria compartilhada.
 *
 * Cont�m apenas os dados da aplica��o, j� que a sincroniza��o
 * ser� feita por objetos nomeados do kernel do Windows.
 */
struct SharedData {
    /// @brief Buffer para armazenar a mensagem (usando wchar_t para compatibilidade com a API Unicode do Windows).
    wchar_t message[MAX_MESSAGE_SIZE];

    /// @brief Flag para sinalizar ao leitor que a comunica��o deve terminar.
    bool exit_requested;
};

// --- Nomes dos Objetos do Kernel ---

/// @brief Nome para o objeto de mapeamento de arquivo (mem�ria compartilhada).
const wchar_t* SHM_NAME = L"MySharedMemoryObject";

/// @brief Nome para o Mutex que proteger� o acesso � mem�ria.
const wchar_t* MUTEX_NAME = L"MySharedMemoryMutex";

/// @brief Nome do Evento que sinaliza que o buffer est� cheio (mensagem pronta para o leitor).
const wchar_t* EVENT_FULL_NAME = L"MySharedMemoryEventFull";

/// @brief Nome do Evento que sinaliza que o buffer est� vazio (pronto para o escritor).
const wchar_t* EVENT_EMPTY_NAME = L"MySharedMemoryEventEmpty";
