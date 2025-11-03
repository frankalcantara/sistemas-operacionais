// WinDiskGeometry.cpp
//
// Exercício 1: O Cliente (User-Mode)
// Demonstra a comunicação Ring 3 -> Ring 0 via System Call.
//
// Compilar com (requer Visual Studio 2022 17.5+):
// cl.exe /EHsc /std:c++latest WinDiskGeometry.cpp
//
// NOTA: /std:c++latest (ou /std:c++23) é necessário para std::expected.
//
// Executar:
// O executável DEVE ser rodado como Administrador para ter permissão
// de abrir o "PhysicalDrive0".

#include <iostream>     // Para std::wcout, std::wcerr
#include <string_view>  // Para std::wstring_view
#include <memory>       // Para std::unique_ptr
#include <iomanip>      // Para std::fixed, std::setprecision
#include <expected>     // O recurso principal do C++23 que estamos usando
#include <windows.h>
#include <winioctl.h>   // Para IOCTL_DISK_GET_DRIVE_GEOMETRY

// 1. RAII: Deleter customizado para o HANDLE do Win32
struct HandleDeleter {
    void operator()(HANDLE h) {
        if (h != INVALID_HANDLE_VALUE && h != nullptr) {
            CloseHandle(h);
        }
    }
};

// Alias para nosso handle gerenciado por RAII
using unique_handle = std::unique_ptr<void, HandleDeleter>;

/**
 * @brief Tenta obter a geometria de um dispositivo de disco.
 * @param device_path O caminho para o dispositivo (ex: L"\\\\.\\PhysicalDrive0").
 * @return Um std::expected contendo DISK_GEOMETRY em caso de sucesso,
 * ou um DWORD (GetLastError()) em caso de falha.
 */
std::expected<DISK_GEOMETRY, DWORD> get_disk_geometry(std::wstring_view device_path) {

    // 2. Obter o handle para o dispositivo (transição de modo iminente)
    unique_handle hDevice(CreateFileW(
        device_path.data(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    ));

    if (hDevice.get() == INVALID_HANDLE_VALUE) {
        // Falha ao obter o handle. Retorna o erro.
        // std::unexpected cria o estado de "erro" do std::expected.
        return std::unexpected(GetLastError());
    }

    // 3. Preparar buffers e chamar DeviceIoControl (A System Call)
    DISK_GEOMETRY diskGeo = {};
    DWORD bytesReturned = 0;

    // Esta é a chamada de sistema que cruza a fronteira Ring 3 -> Ring 0
    BOOL result = DeviceIoControl(
        hDevice.get(),                  // Handle do dispositivo
        IOCTL_DISK_GET_DRIVE_GEOMETRY,  // Código de controle
        nullptr, 0,                     // Sem buffer de entrada
        &diskGeo, sizeof(diskGeo),      // Buffer de saída
        &bytesReturned,                 // Bytes retornados
        nullptr                         // Síncrono
    );

    if (!result) {
        // A chamada de sistema falhou.
        return std::unexpected(GetLastError());
    }

    // 4. Sucesso!
    // std::expected tem um construtor implícito para o tipo "OK".
    return diskGeo;
}

int main() {
    const std::wstring_view path = L"\\\\.\\PhysicalDrive0";

    std::wcout << L"Exercício 1: Cliente User-Mode (Ring 3)\n";
    std::wcout << L"---------------------------------------\n";
    std::wcout << L"Tentando abrir: " << path << L"...\n";

    // 5. Chamar a função e processar o resultado (estilo C++23)
    auto result = get_disk_geometry(path);

    // std::expected pode ser usado diretamente em um 'if'
    if (result) {
        // Sucesso! 'result' contém um valor.
        // Acessamos o valor com o operador *
        const auto& geo = *result;

        std::wcout << L"Chamada de sistema (IOCTL) bem-sucedida.\n";
        std::wcout << L"Driver de disco (Ring 0) retornou a geometria:\n\n";
        std::wcout << L"  Tipo de Mídia:     " << geo.MediaType << L"\n";
        std::wcout << L"  Cilindros:         " << geo.Cylinders.QuadPart << L"\n";
        std::wcout << L"  Trilhas/Cilindro: " << geo.TracksPerCylinder << L"\n";
        std::wcout << L"  Setores/Trilha:    " << geo.SectorsPerTrack << L"\n";
        std::wcout << L"  Bytes/Setor:       " << geo.BytesPerSector << L"\n";

        // Calcular o tamanho total
        ULONGLONG diskSize = geo.Cylinders.QuadPart *
            geo.TracksPerCylinder *
            geo.SectorsPerTrack *
            geo.BytesPerSector;

        double diskSizeGB = static_cast<double>(diskSize) / (1024 * 1024 * 1024);

        std::wcout << L"\n  Tamanho Total:     "
            << std::fixed << std::setprecision(2) << diskSizeGB
            << L" GB" << std::endl;

    }
    else {
        // Falha! 'result' não contém um valor.
        // Acessamos o erro com .error()
        DWORD errorCode = result.error();
        std::wcerr << L"\nFalha na operação. Erro do Win32: " << errorCode << L"\n";

        if (errorCode == ERROR_ACCESS_DENIED) { // Erro 5
            std::wcerr << L"DICA: Este programa deve ser executado como Administrador." << std::endl;
        }
        else if (errorCode == ERROR_FILE_NOT_FOUND) { // Erro 2
            std::wcerr << L"DICA: O dispositivo 'PhysicalDrive0' não foi encontrado." << std::endl;
        }
    }

    std::wcout << L"\n\nPressione ENTER para sair..." << std::endl;
    std::wcin.get(); // Espera pelo caractere 'newline' (Enter)
    
    return 0;
} // Fim do main. O 'unique_handle' sai de escopo e chama CloseHandle() automaticamente.