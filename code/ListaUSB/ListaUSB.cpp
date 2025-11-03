// WinUsbEnum.cpp 
// Compilar com: cl.exe /EHsc /std:c++23 WinUsbEnum.cpp setupapi.lib 
// É necessário vincular à setupapi.lib

#include <iostream> 
#include <string> 
#include <vector> 
#include <memory> // Para std::unique_ptr 
#include <windows.h> 
#include <setupapi.h> 
#include <initguid.h> // Para DEFINE_GUID 
#include <devguid.h> // Para GUID_DEVCLASS_USB

// Um "custom deleter" para std::unique_ptr lidar com HDEVINFO 
struct DevInfoListDeleter {
    void operator()(void* h) {
        if (h != INVALID_HANDLE_VALUE) {
            SetupDiDestroyDeviceInfoList(static_cast<HDEVINFO>(h));
        }
    }
};

using unique_hdevinfo = std::unique_ptr<void, DevInfoListDeleter>;

int main() {
    // 1. Obter o conjunto de informações do dispositivo 
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_USB,
        nullptr,
        nullptr,
        DIGCF_PRESENT
    );

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        // CORREÇÃO: Usar std::wcerr e strings L"" (wide)
        std::wcerr << L"Falha ao chamar SetupDiGetClassDevs. Erro: " << GetLastError() << std::endl;
        return 1;
    }

    unique_hdevinfo hDevInfoManaged(hDevInfo);

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DWORD deviceIndex = 0;

    std::wcout << L"Enumerando dispositivos USB (GUID_DEVCLASS_USB):\n\n";

    // 2. Iterar sobre todos os dispositivos no conjunto 
    while (SetupDiEnumDeviceInfo(hDevInfoManaged.get(), deviceIndex, &devInfoData)) {
        deviceIndex++;
        DWORD requiredSize = 0;
        DWORD propertyType = 0;

        // Primeira chamada: obter o tamanho do buffer necessário 
        SetupDiGetDeviceRegistryProperty(
            hDevInfoManaged.get(), &devInfoData, SPDRP_DEVICEDESC,
            &propertyType, nullptr, 0, &requiredSize
        );

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            // CORREÇÃO: Usar std::wcerr e strings L"" (wide)
            std::wcerr << L" Erro ao obter tamanho da propriedade. Pulando.\n";
            continue;
        }

        // CORREÇÃO DE COMENTÁRIO: 
        // Isto é C++ moderno (pós-C++11), não específico do C++23.
        std::vector<BYTE> propertyBuffer(requiredSize);

        // Segunda chamada: obter os dados da propriedade 
        if (SetupDiGetDeviceRegistryProperty(
            hDevInfoManaged.get(), &devInfoData, SPDRP_DEVICEDESC,
            nullptr, propertyBuffer.data(), requiredSize, nullptr
        )) {
            std::wcout << L"Dispositivo " << deviceIndex << L": "
                << reinterpret_cast<wchar_t*>(propertyBuffer.data()) << std::endl;
        }
        else {
            // CORREÇÃO: Usar std::wcerr e strings L"" (wide)
            std::wcerr << L" Falha ao obter propriedade. Erro: " << GetLastError() << std::endl;
        }
    }

    if (GetLastError() != 0 && GetLastError() != ERROR_NO_MORE_ITEMS) {
        // CORREÇÃO: Usar std::wcerr e strings L"" (wide)
        std::wcerr << L"Erro durante a enumeração: " << GetLastError() << std::endl;
    }

    std::wcout << L"\nEnumeração concluída." << std::endl;

    return 0;
}