/**
 * @file process_monitor.cpp
 * @brief Monitor de processos Windows com listagem e monitoramento em tempo real
 * @author Sistema Operacional - Exemplo Prático
 * @version 1.0
 * @date 2025
 *
 * Este programa demonstra como listar processos ativos no Windows e monitorar
 * o uso de memória de um processo específico em tempo real usando Windows API.
 * Implementa funcionalidades básicas de monitoramento de sistema operacional.
 *
 * @details O código utiliza as estruturas internas do Windows discutidas:
 * - EPROCESS: Acessada indiretamente via handles de processo
 * - VAD Tree: Dados de memória virtual obtidos através das VADs
 * - Memory Manager: Working sets e page faults obtidos do MM
 * - PEB: Informações de módulos via loader data
 */

#include <iostream>     ///< std::cout, std::cerr, std::cin para entrada/saída padrão
#include <vector>       ///< std::vector - contêiner dinâmico (não usado no código atual)
#include <string>       ///< std::string para manipulação de strings ASCII
#include <windows.h>    ///< Windows API principal - HANDLE, DWORD, TCHAR, OpenProcess, CloseHandle
#include <psapi.h>      ///< Process Status API - EnumProcesses, GetProcessMemoryInfo, EnumProcessModules
#include <iomanip>      ///< std::setw, std::setprecision, std::fixed para formatação de saída
#include <chrono>       ///< std::chrono::seconds para controle de tempo no monitoramento
#include <thread>       ///< std::this_thread::sleep_for para pausas entre atualizações
#include <limits>       ///< std::numeric_limits (incluído mas não usado diretamente)

 /**
  * @brief Converte valores em bytes para formato legível com sufixos apropriados
  * @param bytes Quantidade de bytes a ser convertida
  * @return String formatada com o valor e sufixo apropriado (B, KB, MB, GB, TB)
  *
  * Esta função implementa conversão de unidades de memória seguindo o padrão binário
  * (1024 bytes = 1 KB). Útil para exibir informações de memória de forma compreensível
  * ao usuário final, convertendo valores grandes em unidades mais legíveis.
  *
  * @note Usa padrão binário (1024) consistente com como o Memory Manager do Windows
  * calcula working sets e page frames (4096 bytes = 4KB por página).
  */
std::string formatBytes(size_t bytes) {
    // Caso especial: zero bytes
    if (bytes == 0) return "0 B";

    // Array de sufixos para unidades de memória (padrão binário)
    const char* suffixes[] = { "B", "KB", "MB", "GB", "TB" };
    int i = 0;
    double dblBytes = static_cast<double>(bytes);

    // Divide por 1024 até encontrar a unidade apropriada
    // Loop continua enquanto o valor >= 1024 e não excede o array de sufixos
    while (dblBytes >= 1024 && i < 4) {
        dblBytes /= 1024;  ///< Divisão por 1024 (padrão binário para memória)
        i++;
    }

    // Formata o resultado com 2 casas decimais usando stringstream
    std::stringstream stream;
    stream << std::fixed << std::setprecision(2) << dblBytes << " " << suffixes[i];
    return stream.str();
}

/**
 * @brief Lista todos os processos ativos no sistema Windows
 * @return true se a enumeração foi bem-sucedida, false em caso de erro
 *
 * Esta função utiliza a Windows API para enumerar todos os processos em execução.
 * Usa EnumProcesses() para obter lista de PIDs e OpenProcess() para acessar
 * informações de cada processo. Exibe uma tabela formatada com PID e nome.
 *
 * @details Fluxo interno das APIs:
 * 1. EnumProcesses() → psapi.dll → kernel → percorre ActiveProcessLinks (EPROCESS)
 * 2. OpenProcess() → kernel32.dll → NtOpenProcess → localiza EPROCESS pelo PID
 * 3. EnumProcessModules() → acessa PEB.Ldr → InLoadOrderModuleList
 * 4. GetModuleBaseName() → obtém nome do primeiro módulo (executável principal)
 */
bool listarProcessos() {
    // Array estático para armazenar PIDs dos processos
    // Tamanho 2048 suporta até 2048 processos simultâneos
    DWORD aProcesses[2048], cbNeeded, cProcesses;

    // EnumProcesses: enumera todos os processos no sistema
    // Parâmetros: buffer de destino, tamanho do buffer, bytes efetivamente usados
    // INTERNAMENTE: Percorre lista ActiveProcessLinks das estruturas EPROCESS no kernel
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        std::cerr << "Erro ao enumerar processos." << std::endl;
        return false;
    }

    // Calcula o número de processos encontrados
    // cbNeeded contém o número total de bytes usados
    cProcesses = cbNeeded / sizeof(DWORD);

    // Cabeçalho da tabela de processos
    std::cout << "--- Processos Ativos ---\n";
    std::cout << std::left << std::setw(10) << "PID" << std::setw(50) << "Nome do Processo" << "\n";
    std::cout << std::string(60, '-') << "\n";

    // Itera através de todos os PIDs encontrados
    for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {  ///< PID 0 é reservado pelo sistema
            // Buffer para armazenar o nome do processo
            TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

            // OpenProcess: abre handle do processo para consulta
            // PROCESS_QUERY_INFORMATION: permite consultar informações básicas
            // PROCESS_VM_READ: permite ler memória virtual do processo
            // FALSE: handle não é herdável por processos filhos
            // INTERNAMENTE: kernel32.dll → NtOpenProcess → localiza EPROCESS pelo UniqueProcessId
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, aProcesses[i]);

            if (hProcess != NULL) {
                HMODULE hMod;           ///< Handle do módulo (executável principal)
                DWORD cbNeededMod;      ///< Bytes necessários para a operação

                // EnumProcessModules: enumera módulos carregados pelo processo
                // Obtém o primeiro módulo que é o executável principal
                // INTERNAMENTE: Acessa EPROCESS.Peb → PEB.Ldr → InLoadOrderModuleList
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeededMod)) {
                    // GetModuleBaseName: obtém o nome base do módulo (sem caminho)
                    // INTERNAMENTE: Lê LDR_DATA_TABLE_ENTRY.BaseDllName da lista de módulos
                    GetModuleBaseName(hProcess, hMod, szProcessName,
                        sizeof(szProcessName) / sizeof(TCHAR));
                }
            }

            // Exibe PID e nome do processo com formatação tabular
            // std::wcout para suporte a caracteres Unicode (TCHAR)
            std::wcout << std::left << std::setw(10) << aProcesses[i]
                << std::setw(50) << szProcessName << "\n";

            // CloseHandle: libera o handle do processo para evitar vazamentos de recursos
            // Importante: cada OpenProcess deve ter um CloseHandle correspondente
            // INTERNAMENTE: Decrementa reference count do objeto EPROCESS
            if (hProcess != NULL) {
                CloseHandle(hProcess);
            }
        }
    }
    return true;
}

/**
 * @brief Monitora um processo específico em tempo real
 * @param pid Process ID do processo a ser monitorado
 *
 * Esta função implementa monitoramento contínuo de uso de memória de um processo.
 * Atualiza as informações a cada segundo e exibe métricas detalhadas de memória
 * incluindo Working Set, Commit Size e Page Faults. O monitoramento continua
 * até o processo terminar ou o usuário interromper com Ctrl+C.
 *
 * @details Métricas coletadas e suas origens nas estruturas internas:
 * - WorkingSetSize: EPROCESS.WorkingSetSize (calculado pelo Memory Manager)
 * - PrivateUsage: Soma das VAD entries marcadas como Private + Committed
 * - PageFaultCount: EPROCESS.Vm.PageFaultCount (mantido pelo MM)
 * - Página count: WorkingSetSize / 4096 (page size no Windows x86/x64)
 */
void monitorarProcesso(DWORD pid) {
    // Abre handle do processo com permissões de consulta e leitura de memória
    // INTERNAMENTE: Kernel localiza EPROCESS correspondente ao PID fornecido
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "Nao foi possivel abrir o processo com PID " << pid
            << ". Verifique se o PID esta correto e se voce tem permissao." << std::endl;
        return;
    }

    // Estrutura estendida para contadores de memória do processo
    // Contém informações detalhadas sobre uso de memória virtual e física
    // Campos mapeiam diretamente para dados das estruturas EPROCESS e Memory Manager
    PROCESS_MEMORY_COUNTERS_EX pmc;

    // Loop infinito de monitoramento - termina quando processo acaba ou falha
    while (true) {
        // GetProcessMemoryInfo: obtém estatísticas de memória do processo
        // Cast para PROCESS_MEMORY_COUNTERS* para compatibilidade com a API
        // INTERNAMENTE: psapi.dll → kernel → coleta dados do Memory Manager
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            // Limpa a tela para atualização das informações
            // system("cls") é específico do Windows
            system("cls");

            std::cout << "--- Monitorando Processo PID: " << pid << " ---\n\n";

            // Working Set: páginas de memória física atualmente em uso pelo processo
            // Representa a "pegada" atual do processo na RAM
            // ORIGEM: EPROCESS.WorkingSetSize - número de page frames na RAM
            // Calculado pelo Memory Manager baseado nas páginas resident das VADs
            std::cout << std::left << std::setw(35) << "Uso de Memoria Fisica (Working Set): "
                << formatBytes(pmc.WorkingSetSize) << "\n";

            // Private Usage (Commit): memória virtual privada comprometida pelo processo
            // Inclui memória que pode estar em RAM ou arquivo de paginação
            // ORIGEM: Soma de todas as VAD entries com flags Private + Committed
            // Representa o backing store total garantido para este processo
            std::cout << std::left << std::setw(35) << "Memoria Virtual Alocada (Commit): "
                << formatBytes(pmc.PrivateUsage) << "\n";

            // Page Fault Count: número de faltas de página desde o início do processo
            // Indica quantas vezes o processo acessou páginas não residentes na RAM
            // ORIGEM: EPROCESS.Vm.PageFaultCount - mantido pelo Memory Manager
            // Inclui hard faults (disco) e soft faults (standby/modified lists)
            std::cout << std::left << std::setw(35) << "Numero de Page Faults: "
                << pmc.PageFaultCount << "\n";

            // Número de páginas físicas: Working Set dividido pelo tamanho da página
            // Tamanho padrão da página no Windows x86/x64 é 4096 bytes (4 KB)
            // ORIGEM: Número de entradas na Working Set List do processo
            // Cada entrada representa um page frame na Page Frame Database (PFN)
            std::cout << std::left << std::setw(35) << "Paginas de Memoria (Working Set): "
                << pmc.WorkingSetSize / 4096 << "\n";

            std::cout << "\n(Pressione Ctrl+C para parar o monitoramento)";

        }
        else {
            // Falha pode indicar que o processo foi terminado ou perdeu-se acesso
            // GetLastError() forneceria código específico do erro
            // Comum quando EPROCESS é removida da ActiveProcessLinks (processo termina)
            std::cerr << "Nao foi possivel obter informacoes de memoria. O processo "
                << pid << " pode ter sido finalizado." << std::endl;
            break;
        }

        // Pausa de 1 segundo entre atualizações
        // std::chrono::seconds(1) é mais robusto que Sleep(1000)
        // Durante essa pausa, Memory Manager pode fazer working set trimming
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Libera o handle do processo ao sair do loop de monitoramento
    // INTERNAMENTE: Decrementa reference count da estrutura EPROCESS
    CloseHandle(hProcess);
}

/**
 * @brief Função principal do programa
 * @return Código de saída (0 para sucesso, 1 para erro)
 *
 * Coordena a execução do programa: lista processos disponíveis,
 * solicita ao usuário o PID do processo a monitorar e inicia
 * o monitoramento em tempo real.
 *
 * @details Fluxo completo de execução:
 * 1. Configuração de console para UTF-8
 * 2. Enumeração de processos (acesso às estruturas EPROCESS)
 * 3. Seleção de PID pelo usuário
 * 4. Monitoramento contínuo via Memory Manager APIs
 */
int main() {
    // SetConsoleOutputCP: configura codificação de saída do console para UTF-8
    // Necessário para exibição correta de caracteres especiais no Windows
    // Permite mostrar nomes de processos com caracteres não-ASCII
    SetConsoleOutputCP(CP_UTF8);

    // Lista todos os processos ativos do sistema
    // INTERNAMENTE: Enumera ActiveProcessLinks da lista global de EPROCESS
    if (!listarProcessos()) {
        return 1;  ///< Sai com código de erro se a listagem falhar
    }

    // Variável para armazenar o PID selecionado pelo usuário
    DWORD pidSelecionado = 0;
    std::cout << "\n\nDigite o PID do processo que deseja monitorar: ";
    std::cin >> pidSelecionado;

    // Validação da entrada do usuário
    // std::cin.fail() detecta falhas na conversão (entrada não numérica)
    // PID 0 é reservado pelo sistema (Idle Process)
    if (std::cin.fail() || pidSelecionado == 0) {
        std::cerr << "Entrada invalida. Por favor, insira um numero de PID valido." << std::endl;
        return 1;
    }

    // Inicia o monitoramento do processo selecionado
    // Estabelece conexão contínua com as estruturas do Memory Manager
    monitorarProcesso(pidSelecionado);

    return 0;
}

/**
 * @note Conexões com Estruturas Internas do Windows:
 *
 * EPROCESS (Executive Process Block):
 * - UniqueProcessId: Usado por OpenProcess() para localizar o processo
 * - WorkingSetSize: Retornado diretamente em PROCESS_MEMORY_COUNTERS_EX
 * - Peb: Usado por EnumProcessModules() para acessar lista de módulos
 * - VadRoot: Usado para calcular PrivateUsage (soma das VAD committed)
 *
 * VAD Tree (Virtual Address Descriptors):
 * - Cada nó representa uma região de memória virtual alocada
 * - Flags indicam se a região é Private (contribui para PrivateUsage)
 * - CommitCharge de cada VAD soma para o total committed do processo
 *
 * Memory Manager:
 * - Page Frame Database (PFN): Rastreia quais páginas estão na RAM
 * - Working Set Lists: Determinam o WorkingSetSize do processo
 * - Page Fault Handler: Incrementa PageFaultCount a cada fault
 *
 * PEB (Process Environment Block):
 * - Ldr.InLoadOrderModuleList: Usado por EnumProcessModules()
 * - Cada LDR_DATA_TABLE_ENTRY contém nome e endereço base dos módulos
 *
 * Backing Store:
 * - PrivateUsage representa memória com backing store garantido
 * - Pode estar na RAM (WorkingSet) ou no page file
 * - Diferença entre WorkingSet e PrivateUsage mostra quanto está paginado
 */