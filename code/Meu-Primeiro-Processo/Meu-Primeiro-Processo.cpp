/**
 * @file process_lister.cpp
 * @brief Programa para listar processos em execução no **Windows** 11
 * @author Livro de Sistemas Operacionais
 * @version 1.
 * @date 2025
 *
 * Este programa demonstra como enumerar processos em execução usando
 * a **Windows** API em C++23.
 */

#include <windows.h>    // API principal do **Windows** - fornece tipos básicos (DWORD, HANDLE) 
 // e funções do sistema como OpenProcess(), CloseHandle()

#include <psapi.h>      // Process Status API - funções específicas para enumeração de processos:
                        // EnumProcesses(), EnumProcessModules(), GetModuleBaseNameW(),
                        // GetProcessMemoryInfo(), QueryFullProcessImageNameW()

#include <iostream>     // Fluxos de entrada/saída padrão - std::wcout, std::wcerr, std::wcin
                        // para exibição de texto Unicode na console

#include <vector>       // Contêiner dinâmico std::vector para armazenar listas de ProcessInfo
                        // e arrays de IDs de processos retornados pela API

#include <string>       // std::wstring para manipulação de strings Unicode (nomes e caminhos
                        // de processos que podem conter caracteres especiais)

#include <format>       // std::format (C++20) - formatação moderna de strings tipo printf
                        // mas type-safe para criar tabelas e exibições formatadas

#include <ranges>       // std::ranges (C++20) - algoritmos funcionais modernos como
                        // ranges::sort(), ranges::copy_if() para manipulação de dados

#include <algorithm>    // Algoritmos STL complementares e std::back_inserter para
                        // operações auxiliares de manipulação de contêineres

#include <memory>       // Smart pointers e utilitários de gerenciamento de memória
                        // (incluído para possíveis extensões futuras do código)

// não espere que eu vá comentar as bibliotecas novamente. Esta foi uma concessão por ser 
// nosso primeiro código.

/**
 * @brief Estrutura para armazenar informações de um processo
 *
 * Esta estrutura encapsula as informações básicas de um processo
 * que serão coletadas e exibidas pelo programa.
 */
struct ProcessInfo {
    DWORD processId;           ///< ID do processo (PID)
    std::wstring processName;  ///< Nome do executável do processo
    std::wstring fullPath;     ///< Caminho completo do executável
    SIZE_T workingSetSize;     ///< Tamanho do working set em bytes

    /**
    O Tamanho do working representa a quantidade de memória física (RAM) que está atualmente sendo utilizada por um processo específico. Em **Sistemas Operacionais** com memória virtual como o Windows, o working set é o conjunto de páginas de memória que estão fisicamente residentes na `RAM` para aquele processo em um determinado momento
    **/

    /**
     * @brief Construtor padrão
     */
    ProcessInfo() : processId(0), workingSetSize(0) {}

    /**
     * @brief Construtor com parâmetros
     * @param **PID** ID do processo
     * @param name Nome do processo
     * @param path Caminho completo
     * @param memSize Tamanho da memória em uso
     */
    ProcessInfo(DWORD pid, const std::wstring& name,
        const std::wstring& path, SIZE_T memSize)
        : processId(pid), processName(name), fullPath(path), workingSetSize(memSize) {
    }
};

/**
 * @brief Classe responsável por enumerar e gerenciar informações de processos
 *
 * Esta classe encapsula toda a funcionalidade relacionada à coleta
 * de informações sobre processos em execução no sistema Windows.
 */
class ProcessEnumerator {
private:
    std::vector<ProcessInfo> processes; ///< Lista de processos coletados

    /**
     * @brief Obtém o nome do processo a partir de seu handle
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
     * @brief Obtém o caminho completo do executável do processo
     * @param hProcess Handle do processo
     * @return Caminho completo ou string vazia em caso de erro
     */
    std::wstring getProcessPath(HANDLE hProcess) const {
        wchar_t processPath[MAX_PATH] = L"<caminho não disponível>";

        if (hProcess != nullptr) {
            DWORD pathLength = MAX_PATH;
            if (!QueryFullProcessImageNameW(hProcess, 0, processPath, &pathLength)) {
                wcscpy_s(processPath, L"<acesso negado>");
            }
        }

        return std::wstring(processPath);
    }

    /**
     * @brief Obtém informações de memória do processo
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
     * @brief Enumera todos os processos em execução no sistema
     * @return true se a enumeração foi bem-sucedida, false caso contrário
     *
     * Este método coleta informações sobre todos os processos em execução
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

        // Calcula o número de processos retornados
        DWORD processCount = bytesReturned / sizeof(DWORD);

        // Processa cada ID de processo
        for (DWORD i = 0; i < processCount; ++i) {
            DWORD processId = processIds[i];

            // Abre o processo com permissões limitadas
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, processId);

            if (hProcess != nullptr) {
                // Coleta informações do processo
                auto name = getProcessName(hProcess);
                auto path = getProcessPath(hProcess);
                auto memSize = getProcessMemoryInfo(hProcess);

                // Adiciona à lista usando emplace_back (C++ 11+)
                processes.emplace_back(processId, name, path, memSize);

                CloseHandle(hProcess);
            }
            else {
                // Processo sem acesso - adiciona informações básicas
                processes.emplace_back(processId, L"<acesso negado>",
                    L"<acesso negado>", 0);
            }
        }

        return true;
    }

    /**
     * @brief Exibe a lista de processos formatada
     *
     * Utiliza std::format (C++ 20) e ranges (C++ 20) para formatação
     * e manipulação dos dados de forma moderna.
     */
    void displayProcesses() const {
        // Cabeçalho da tabela
        std::wcout << std::format(L"{:>8} | {:30} | {:10} | {}\n",
            L"PID", L"Nome do Processo", L"Memória (KB)", L"Caminho");
        std::wcout << std::wstring(80, L'-') << std::endl;

        // Ordena os processos por **PID** usando ranges (C++ 20)
		// para o código ficar simples, teremos um warning aqui, mas é aceitável
        // eu quero só uma lista de processos para poder ordenar, então esta 
		// é uma solução simples, estúpida e rápida para esta necessidade. 
		// evite cópias sempre que possível.
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
     * @brief Obtém estatísticas dos processos
     * @return Par contendo número total de processos e uso total de memória
     */
    std::pair<size_t, SIZE_T> getStatistics() const {
        SIZE_T totalMemory = 0;

        // Usa ranges::fold_left (C++23) para somar a memória total
        for (const auto& proc : processes) {
            totalMemory += proc.workingSetSize;
        }

        return { processes.size(), totalMemory };
    }

    /**
     * @brief Filtra processos por nome
     * @param namePattern Padrão do nome para filtrar
     * @return Vetor com processos que correspondem ao padrão
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
 * @brief Função principal do programa
 * @return Código de saída (0 para sucesso)
 *
 * Demonstra o uso da classe ProcessEnumerator para listar
 * processos em execução no sistema Windows.
 */
int main() {
    // Configura a saída para suportar caracteres Unicode
    std::wcout.imbue(std::locale(""));

    std::wcout << L"=== Listador de Processos **Windows** - C++23 ===" << std::endl;
    std::wcout << L"Coletando informações dos processos..." << std::endl << std::endl;

    // Cria o enumerador de processos
    ProcessEnumerator enumerator;

    // Enumera os processos
    if (!enumerator.enumerateProcesses()) {
        std::wcerr << L"Falha ao enumerar processos!" << std::endl;
        return 1;
    }

    // Exibe os processos
    enumerator.displayProcesses();

    // Exibe estatísticas
    auto [processCount, totalMemory] = enumerator.getStatistics();
    std::wcout << std::format(L"\nEstatísticas:\n");
    std::wcout << std::format(L"- Processos em execução: {}\n", processCount);
    std::wcout << std::format(L"- Memória total em uso: {:.0f} MB\n",
        static_cast<double>(totalMemory) / (1024 * 1024));

    // Exemplo de filtragem (opcional)
    std::wcout << L"\nPressione Enter para ver exemplo de filtragem...";
    std::wcin.get();

    auto svcProcesses = enumerator.filterByName(L"svc");
    if (!svcProcesses.empty()) {
        std::wcout << std::format(L"\nProcessos com 'svc' no nome ({} encontrados):\n",
            svcProcesses.size());
        for (const auto& proc : svcProcesses) {
            std::wcout << std::format(L"- **PID** {}: {}\n",
                proc.processId, proc.processName);
        }
    }

    return 0;
}