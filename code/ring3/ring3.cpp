// WinDeviceEnum.cpp
//
// Exercício 2: O Enumerador (User-Mode)
// Demonstra como a SetupApi age como uma "Camada de Abstração" 
// para consultar o Gerenciador PnP (Kernel) sobre dispositivos.
//
// Compilar com (requer Visual Studio 2022 17.5+):
// cl.exe /EHsc /std:c++latest WinDeviceEnum.cpp setupapi.lib
//
// NOTA: /std:c++latest (ou /std:c++23) é necessário para std::expected.
// É OBRIGATÓRIO vincular setupapi.lib [cite: 85-87] (indiretamente)

#include <iostream>     // Para std::wcout, std::wcerr
#include <string>       // Para std::wstring
#include <vector>       // Para std::vector
#include <memory>       // Para std::unique_ptr
#include <expected>     // O recurso principal do C++23
#include <conio.h>      // Para _getch()
#include <windows.h>
#include <setupapi.h>   // Para as funções SetupDi*
#include <initguid.h>   // Para DEFINE_GUID
#include <devguid.h>    // Para GUID_DEVCLASS_USB

// 1. RAII: Deleter customizado para o HDEVINFO
struct DevInfoListDeleter {
    void operator()(void* h) {
        if (h != INVALID_HANDLE_VALUE) {
            SetupDiDestroyDeviceInfoList(static_cast<HDEVINFO>(h));
        }
    }
};

// Alias para nosso handle gerenciado por RAII
using unique_hdevinfo = std::unique_ptr<void, DevInfoListDeleter>;

/**
 * @brief Obtém uma propriedade de string de um dispositivo.
 * @return Um std::expected contendo a std::wstring em caso de sucesso,
 * ou um DWORD (GetLastError()) em caso de falha.
 */
std::expected<std::wstring, DWORD> GetDeviceStringProperty(
    HDEVINFO hDevInfo,
    SP_DEVINFO_DATA& devInfoData,
    DWORD propertyId)
{
    DWORD requiredSize = 0;
    DWORD propertyType = 0; // Esperamos REG_SZ

    // 2. Primeira chamada (User-Mode -> Kernel-Mode)
    // Pergunta ao kernel: "Qual o tamanho do buffer para esta propriedade?"
    SetupDiGetDeviceRegistryProperty(
        hDevInfo, &devInfoData, propertyId,
        &propertyType, nullptr, 0, &requiredSize
    );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        // A propriedade pode simplesmente não existir
        return std::unexpected(GetLastError());
    }

    if (propertyType != REG_SZ) {
        // Pedimos uma string, mas recebemos outro tipo de dado
        return std::unexpected(ERROR_INVALID_DATATYPE);
    }

    // 3. Alocação de buffer C++ moderno
    std::vector<BYTE> propertyBuffer(requiredSize);

    // 4. Segunda chamada (User-Mode -> Kernel-Mode)
    // "Preencha este buffer com os dados da propriedade"
    if (!SetupDiGetDeviceRegistryProperty(
        hDevInfo, &devInfoData, propertyId,
        nullptr, propertyBuffer.data(), requiredSize, nullptr
    )) {
        return std::unexpected(GetLastError());
    }

    // Converte o buffer (wchar_t*) para std::wstring
    return std::wstring(reinterpret_cast<wchar_t*>(propertyBuffer.data()));
}


int main() {
    std::wcout << L"Exercício 2: O Enumerador (User-Mode)\n";
    std::wcout << L"---------------------------------------\n";
    std::wcout << L"Consultando o PnP Manager (Kernel) via SetupApi...\n";
    std::wcout << L"Listando dispositivos da classe: GUID_DEVCLASS_USB\n\n";

    // 5. Obter o conjunto de dispositivos (Device Information Set)
    // Esta é a chamada principal para a camada de abstração.
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_USB, // Queremos dispositivos da classe USB
        nullptr,            // Sem enumerador específico
        nullptr,            // Sem janela pai
        DIGCF_PRESENT       // Apenas dispositivos atualmente presentes
    );

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Falha ao chamar SetupDiGetClassDevs. Erro: " << GetLastError() << std::endl;
        return 1;
    }

    // Garante que SetupDiDestroyDeviceInfoList será chamado no final
    unique_hdevinfo hDevInfoManaged(hDevInfo);

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DWORD deviceIndex = 0;

    // 6. Iterar sobre todos os dispositivos encontrados
    while (SetupDiEnumDeviceInfo(hDevInfoManaged.get(), deviceIndex, &devInfoData)) {
        deviceIndex++;

        // 7. Obter propriedades usando nossa função C++23
        auto description = GetDeviceStringProperty(
            hDevInfoManaged.get(),
            devInfoData,
            SPDRP_DEVICEDESC // Pedindo a "Descrição do Dispositivo"
        );

        std::wcout << L"Dispositivo " << deviceIndex << L": ";

        if (description) {
            std::wcout << *description << std::endl;
        }
        else {
            // description.error() contém o DWORD do erro
            std::wcerr << L"Falha ao obter descrição. Erro: "
                << description.error() << std::endl;
        }

        // BÔNUS: Tenta obter o fabricante (mesma função, ID diferente)
        auto manufacturer = GetDeviceStringProperty(
            hDevInfoManaged.get(),
            devInfoData,
            SPDRP_MFG // Pedindo o "Fabricante"
        );

        // Imprime o fabricante se a chamada foi bem-sucedida
        if (manufacturer && !manufacturer->empty()) {
            std::wcout << L"  Fabricante: " << *manufacturer << std::endl;
        }
    }

    // Verifica se o loop 'while' parou por um erro ou por terminar a lista
    if (GetLastError() != 0 && GetLastError() != ERROR_NO_MORE_ITEMS) {
        std::wcerr << L"Erro durante a enumeração: " << GetLastError() << std::endl;
    }

    std::wcout << L"\nEnumeração concluída." << std::endl;

    // 8. Pausa para o usuário ler a saída
    std::wcout << L"\n\nPressione qualquer tecla para sair..." << std::endl;
    _getch();

    return 0;
} // hDevInfoManaged sai de escopo aqui, chamando SetupDiDestroyDeviceInfoList