/**
 * @file process_lister.cpp
 * @brief Programa para listar processos em execu��o no Windows 11
 * @author Livro de Sistemas Operacionais
 * @version 1.0
 * @date 2025
 *
 * Este programa demonstra como enumerar processos em execu��o usando
 * a Windows API em C++ 23. � adequado para estudo de sistemas operacionais
 * e demonstra conceitos de gerenciamento de processos.
 */

#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <ranges>
#include <algorithm>
#include <memory>

 /**
  * @brief Estrutura para armazenar informa��es de um processo
  *
  * Esta estrutura encapsula as informa��es b�sicas de um processo
  * que ser�o coletadas e exibidas pelo programa.
  */
struct ProcessInfo {
    DWORD processId;           ///< ID do processo (PID)
    std::wstring processName;  ///< Nome do execut�vel do processo
    std::wstring fullPath;     ///< Caminho completo do execut�vel
    SIZE_T workingSetSize;     ///< Tamanho do working set em bytes

    /**
     * @brief Construtor padr�o
     */
    ProcessInfo() : processId(0), workingSetSize(0) {}

    /**
     * @brief Construtor com par�metros
     * @param pid ID do processo
     * @param name Nome do processo
     * @param path Caminho completo
     * @param memSize Tamanho da mem�ria em uso
     */
    ProcessInfo(DWORD pid, const std::wstring& name,
        const std::wstring& path, SIZE_T memSize)
        : processId(pid), processName(name), fullPath(path), workingSetSize(memSize) {
    }
};

/**
 * @brief Classe respons�vel por enumerar e gerenciar informa��es de processos
 *
 * Esta classe encapsula toda a funcionalidade relacionada � coleta
 * de informa��es sobre processos em execu��o no sistema Windows.
 */
class ProcessEnumerator {
private:
    std::vector<ProcessInfo> processes; ///< Lista de processos coletados

    /**
     * @brief Obt�m o nome do processo a partir de seu handle
     * @param hProcess Handle do processo
     * @return Nome do processo ou string vazia em caso de erro
     */
    std::wstring getProcessName(HANDLE hProcess) const {
        wchar_t processName[MAX_PATH] = L"<desconhecido>";

        if (hProcess != nullptr) {
            HMODULE hMod;
            DWORD cbNeeded;

            if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                GetModuleBaseNameW(hProcess, hMod, processName,
                    sizeof(processName) / sizeof(wchar_t));
            }
        }

        return std::wstring(processName);
    }

    /**
     * @brief Obt�m o caminho completo do execut�vel do processo
     * @param hProcess Handle do processo
     * @return Caminho completo ou string vazia em caso de erro
     */
    std::wstring getProcessPath(HANDLE hProcess) const {
        wchar_t processPath[MAX_PATH] = L"<caminho n�o dispon�vel>";

        if (hProcess != nullptr) {
            DWORD pathLength = MAX_PATH;
            if (!QueryFullProcessImageNameW(hProcess, 0, processPath, &pathLength)) {
                wcscpy_s(processPath, L"<acesso negado>");
            }
        }

        return std::wstring(processPath);
    }

    /**
     * @brief Obt�m informa��es de mem�ria do processo
     * @param hProcess Handle do processo
     * @return Tamanho do working set em bytes
     */
    SIZE_T getProcessMemoryInfo(HANDLE hProcess) const {
        PROCESS_MEMORY_COUNTERS pmc;

        if (hProcess != nullptr && GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }

        return 0;
    }

public:
    /**
     * @brief Enumera todos os processos em execu��o no sistema
     * @return true se a enumera��o foi bem-sucedida, false caso contr�rio
     *
     * Este m�todo coleta informa��es sobre todos os processos em execu��o
     * usando as `APIs`EnumProcesses e OpenProcess do Windows.
     */
    bool enumerateProcesses() {
        // Limpa a lista anterior
        processes.clear();

        // Buffer para armazenar os IDs dos processos
        std::vector<DWORD> processIds(1024);
        DWORD bytesReturned;

        // Enumera todos os processos
        if (!EnumProcesses(processIds.data(),
            static_cast<DWORD>(processIds.size() * sizeof(DWORD)),
            &bytesReturned)) {
            std::wcerr << L"Erro ao enumerar processos: " << GetLastError() << std::endl;
            return false;
        }

        // Calcula o n�mero de processos retornados
        DWORD processCount = bytesReturned / sizeof(DWORD);

        // Processa cada ID de processo
        for (DWORD i = 0; i < processCount; ++i) {
            DWORD processId = processIds[i];

            // Abre o processo com permiss�es limitadas
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, processId);

            if (hProcess != nullptr) {
                // Coleta informa��es do processo
                auto name = getProcessName(hProcess);
                auto path = getProcessPath(hProcess);
                auto memSize = getProcessMemoryInfo(hProcess);

                // Adiciona � lista usando emplace_back (C++ 11+)
                processes.emplace_back(processId, name, path, memSize);

                CloseHandle(hProcess);
            }
            else {
                // Processo sem acesso - adiciona informa��es b�sicas
                processes.emplace_back(processId, L"<acesso negado>",
                    L"<acesso negado>", 0);
            }
        }

        return true;
    }

    /**
     * @brief Exibe a lista de processos formatada
     *
     * Utiliza std::format (C++ 20) e ranges (C++ 20) para formata��o
     * e manipula��o dos dados de forma moderna.
     */
    void displayProcesses() const {
        // Cabe�alho da tabela
        std::wcout << std::format(L"{:>8} | {:30} | {:10} | {}\n",
            L"PID", L"Nome do Processo", L"Mem�ria (KB)", L"Caminho");
        std::wcout << std::wstring(80, L'-') << std::endl;

        // Ordena os processos por PID usando ranges (C++ 20)
        auto sortedProcesses = processes;
        std::ranges::sort(sortedProcesses,
            [](const ProcessInfo& a, const ProcessInfo& b) {
                return a.processId < b.processId;
            });

        // Exibe cada processo
        for (const auto& proc : sortedProcesses) {
            // Converte bytes para KB
            SIZE_T memoryKB = proc.workingSetSize / 1024;

            std::wcout << std::format(L"{:8} | {:30} | {:10} | {}\n",
                proc.processId,
                proc.processName.substr(0, 30),
                memoryKB,
                proc.fullPath);
        }

        std::wcout << std::format(L"\nTotal de processos: {}\n", processes.size());
    }

    /**
     * @brief Obt�m estat�sticas dos processos
     * @return Par contendo n�mero total de processos e uso total de mem�ria
     */
    std::pair<size_t, SIZE_T> getStatistics() const {
        SIZE_T totalMemory = 0;

        // Usa ranges::fold_left (C++ 23) para somar a mem�ria total
        for (const auto& proc : processes) {
            totalMemory += proc.workingSetSize;
        }

        return { processes.size(), totalMemory };
    }

    /**
     * @brief Filtra processos por nome
     * @param namePattern Padr�o do nome para filtrar
     * @return Vetor com processos que correspondem ao padr�o
     */
    std::vector<ProcessInfo> filterByName(const std::wstring& namePattern) const {
        std::vector<ProcessInfo> filtered;

        // Usa ranges::copy_if (C++ 20) para filtragem
        std::ranges::copy_if(processes, std::back_inserter(filtered),
            [&namePattern](const ProcessInfo& proc) {
                return proc.processName.find(namePattern) != std::wstring::npos;
            });

        return filtered;
    }
};

/**
 * @brief Fun��o principal do programa
 * @return C�digo de sa�da (0 para sucesso)
 *
 * Demonstra o uso da classe ProcessEnumerator para listar
 * processos em execu��o no sistema Windows.
 */
int main() {
    // Configura a sa�da para suportar caracteres Unicode
    std::wcout.imbue(std::locale(""));

    std::wcout << L"=== Listador de Processos Windows - C++ 23 ===" << std::endl;
    std::wcout << L"Coletando informa��es dos processos..." << std::endl << std::endl;

    // Cria o enumerador de processos
    ProcessEnumerator enumerator;

    // Enumera os processos
    if (!enumerator.enumerateProcesses()) {
        std::wcerr << L"Falha ao enumerar processos!" << std::endl;
        return 1;
    }

    // Exibe os processos
    enumerator.displayProcesses();

    // Exibe estat�sticas
    auto [processCount, totalMemory] = enumerator.getStatistics();
    std::wcout << std::format(L"\nEstat�sticas:\n");
    std::wcout << std::format(L"- Processos em execu��o: {}\n", processCount);
    std::wcout << std::format(L"- Mem�ria total em uso: {:.2f} MB\n",
        static_cast<double>(totalMemory) / (1024 * 1024));

    // Exemplo de filtragem (opcional)
    std::wcout << L"\nPressione Enter para ver exemplo de filtragem...";
    std::wcin.get();

    auto svcProcesses = enumerator.filterByName(L"svc");
    if (!svcProcesses.empty()) {
        std::wcout << std::format(L"\nProcessos com 'svc' no nome ({} encontrados):\n",
            svcProcesses.size());
        for (const auto& proc : svcProcesses) {
            std::wcout << std::format(L"- PID {}: {}\n",
                proc.processId, proc.processName);
        }
    }

    return 0;
}