## Simulador 1: Simulando Gerenciamento de Processos **Linux** Mudar para depois do escalonamento

Compreender o funcionamento interno do `kernel` **Linux** é uma tarefa desafiadora, especialmente quando se trata de conceitos fundamentais como **P**rocess **C**ontrol **B**lock, **PCB**, hierarquia de processos e escalonamento. Para ajudar a dedicada leitora a visualizar esses mecanismos complexos, vamos criar um simulador que demonstra o funcionamento da `task_struct` e suas estruturas auxiliares. Eu optei por usar **C++23** porque oferece recursos modernos ideais para modelar sistemas complexos.

O gerenciamento de processos opera em nível de `kernel` de forma transparente ao usuário, tornando seus mecanismos invisíveis ao programador comum. Um simulador permite visualizar e quantificar esse comportamento oculto, demonstrando como diferentes operações (fork, clone, exec, exit) impactam as estruturas de dados do `kernel` e como o escalonador toma decisões baseadas em métricas como `vruntime`, `deadline` e `slice`.

A fórmula do **EEVDF**:

$$\text{Virtual Deadline} = \text{Eligible Time} + \frac{\text{Time Slice}}{\text{Weight}}$$

é matematicamente elegante, mas o simulador converte teoria abstrata em resultados tangíveis. A curiosa leitora poderá experimentar com diferentes configurações de prioridade e observar como o escalonador seleciona a próxima tarefa baseado em lag e deadlines virtuais.

O conceito de **hierarquia de processos** é difícil de intuir sem visualização. O simulador demonstrará concretamente como `PPID`, `PID` e `TGID` se relacionam, como threads compartilham estruturas via `CLONE_*` flags, e como o compartilhamento de `mm_struct`, `files_struct` e `signal_struct` funciona na prática. O simulador também permitirá a exploração de diferentes políticas de escalonamento, estados de processo e transições, analisando os custos computacionais de cada operação.

Finalmente, desenvolvedores de sistemas precisam entender como suas decisões arquiteturais afetam o `kernel` subjacente. O simulador servirá como um laboratório seguro para experimentar conceitos de IPC, sincronização e gerenciamento de recursos antes de aplicá-los em sistemas reais. A @fig-simul2 ilustra o fluxograma do simulador.

::: {#fig-simul2}
![](/images/process_simulator_flowchart.webp)

Fluxograma do simulador de processos demonstrando criação, escalonamento e gerenciamento de estruturas do `kernel`.
:::

O diagrama da @fig-simul2 ilustra como cada operação do sistema percorre as estruturas fundamentais do kernel, desde a criação de novos processos via `fork()`/`clone()` até as decisões críticas do escalonador **EEVDF**. O caminho azul (**Process Creation**) representa a alocação de novas `task_struct` e estruturas auxiliares, enquanto o caminho verde (**Scheduling Decision**) mostra o algoritmo de seleção baseado em lag e virtual deadlines. Os elementos informativos destacam o impacto da hierarquia de processos, compartilhamento de estruturas via `CLONE_*` flags, e as transições de estado. A fórmula implementada demonstra como o EEVDF calcula prioridades, evidenciando como tarefas latency-sensitive com time slices menores obtêm deadlines mais próximos. O laço de controle representa a simulação contínua do escalonador, culminando na análise que valida os conceitos teóricos através de implementação prática. Este fluxograma está implementado no @lst-simul2.

::: {#lst-simul2}

```cpp
/**
 * @file process_management_simulator.cpp
 * @brief Simulador de gerenciamento de processos do `kernel` Linux
 * @author Livro de Sistemas Operacionais
 * @version 1.0
 * @date 2025
 *
 * Este programa demonstra o funcionamento da task_struct e estruturas
 * auxiliares (mm_struct, files_struct, signal_struct, thread_struct),
 * implementando um escalonador simplificado baseado em EEVDF e 
 * simulando operações fundamentais como fork(), clone(), exec() e exit().
 *
 * O programa modela a hierarquia de processos, compartilhamento de recursos
 * via CLONE_* flags, e transições de estado, oferecendo visualização
 * completa dos mecanismos internos do `kernel` Linux.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <format>
#include <ranges>
#include <algorithm>
#include <variant>
#include <expected>
#include <optional>
#include <bitset>
#include <queue>
#include <random>

/**
 * @brief Estados possíveis de um processo/thread
 */
enum class ProcessState {
    TASK_RUNNING,        ///< Executando ou na fila de prontos
    TASK_INTERRUPTIBLE,  ///< Dormindo, pode ser acordado por sinais
    TASK_UNINTERRUPTIBLE,///< Dormindo, não pode ser interrompido
    TASK_ZOMBIE,         ///< Processo terminou, aguardando coleta
    TASK_STOPPED         ///< Processo parado por debugging
};

/**
 * @brief Flags para clone() simulando comportamento do kernel
 */
enum class CloneFlags : uint32_t {
    CLONE_VM     = 0x00000100,  ///< Compartilha espaço de endereçamento
    CLONE_FILES  = 0x00000400,  ///< Compartilha tabela de file descriptors
    CLONE_SIGHAND = 0x00000800, ///< Compartilha handlers de sinais
    CLONE_THREAD = 0x00010000   ///< Cria thread no mesmo processo
};

/**
 * @brief Estrutura simulando uma VMA (Virtual Memory Area)
 */
struct VirtualMemoryArea {
    uint64_t start;      ///< Endereço inicial
    uint64_t end;        ///< Endereço final
    uint32_t permissions;///< Permissões (read/write/exec)
    std::string name;    ///< Nome da região (heap, stack, etc.)
};

/**
 * @brief Simulação da mm_struct - gerenciamento de memória
 */
class MemoryManager {
private:
    std::atomic<int> mm_users_;    ///< Threads ativas usando este espaço
    std::atomic<int> mm_count_;    ///< Referências totais à estrutura
    std::vector<VirtualMemoryArea> vmas_; ///< Áreas de memória virtual
    uint64_t pgd_;                 ///< Simulação da tabela de páginas

public:
    /**
     * @brief Construtor inicializando estrutura de memória
     */
    MemoryManager() : mm_users_(1), mm_count_(1), pgd_(0x1000) {
        // Inicializa VMAs básicas (text, data, heap, stack)
        vmas_.emplace_back(0x400000, 0x401000, 0x5, "text");
        vmas_.emplace_back(0x600000, 0x601000, 0x3, "data");
        vmas_.emplace_back(0x602000, 0x800000, 0x3, "heap");
        vmas_.emplace_back(0x7fff000000, 0x7fff800000, 0x3, "stack");
    }

    /**
     * @brief Incrementa contador de usuários
     */
    void addUser() { mm_users_++; }

    /**
     * @brief Decrementa contador de usuários
     * @return true se não há mais usuários
     */
    bool removeUser() { return --mm_users_ == 0; }

    /**
     * @brief Obtém contadores atuais
     */
    std::pair<int, int> getCounters() const {
        return {mm_users_.load(), mm_count_.load()};
    }

    /**
     * @brief Lista VMAs usando C++23 ranges
     */
    auto getExecutableVMAs() const {
        return vmas_ | std::views::filter([](const auto& vma) {
            return vma.permissions & 0x4; // Bit de execução
        });
    }
};

/**
 * @brief Simulação de arquivo aberto
 */
struct OpenFile {
    std::string path;        ///< Caminho do arquivo
    uint32_t flags;         ///< Flags de abertura
    uint64_t position;      ///< Posição atual de leitura/escrita
    bool close_on_exec;     ///< Flag close-on-exec
};

/**
 * @brief Simulação da files_struct - tabela de file descriptors
 */
class FileDescriptors {
private:
    std::atomic<int> count_;                           ///< Contador de referências
    std::vector<std::optional<OpenFile>> fd_array_;   ///< Array de FDs
    std::bitset<256> open_fds_;                       ///< Bitmap de FDs abertos
    std::bitset<256> close_on_exec_;                  ///< Bitmap close-on-exec

public:
    /**
     * @brief Construtor inicializando FDs padrão (stdin, stdout, stderr)
     */
    FileDescriptors() : count_(1) {
        fd_array_.resize(256);
        
        // Inicializa FDs padrão
        fd_array_[0] = OpenFile{"/dev/stdin", 0, 0, false};
        fd_array_[1] = OpenFile{"/dev/stdout", 1, 0, false};
        fd_array_[2] = OpenFile{"/dev/stderr", 1, 0, false};
        
        open_fds_.set(0); open_fds_.set(1); open_fds_.set(2);
    }

    /**
     * @brief Aloca novo file descriptor
     * @param file Arquivo a ser associado
     * @return FD alocado ou erro
     */
    std::expected<int, std::string> allocateFD(const OpenFile& file) {
        // Busca primeiro FD livre usando C++23 ranges
        auto free_fds = std::views::iota(3, 256) 
                      | std::views::filter([this](int fd) { 
                          return !open_fds_[fd]; 
                        });
        
        if (free_fds.empty()) {
            return std::unexpected("No free file descriptors");
        }
        
        int fd = *free_fds.begin();
        fd_array_[fd] = file;
        open_fds_.set(fd);
        
        return fd;
    }

    /**
     * @brief Compartilha tabela com outro processo (CLONE_FILES)
     */
    void share() { count_++; }

    /**
     * @brief Remove referência à tabela
     */
    bool release() { return --count_ == 0; }

    /**
     * @brief Obtém contadores e estatísticas
     */
    auto getStats() const {
        return std::make_tuple(count_.load(), open_fds_.count(), 
                              close_on_exec_.count());
    }
};

/**
 * @brief Simulação da signal_struct - gerenciamento de sinais
 */
class SignalManager {
private:
    std::atomic<int> count_;                    ///< Contador de referências
    std::array<std::function<void()>, 64> handlers_; ///< Handlers de sinais
    std::bitset<64> pending_signals_;          ///< Sinais pendentes

public:
    /**
     * @brief Construtor inicializando handlers padrão
     */
    SignalManager() : count_(1) {
        // Inicializa handlers padrão (SIG_DFL para a maioria)
        for (auto& handler : handlers_) {
            handler = []() { /* SIG_DFL */ };
        }
    }

    /**
     * @brief Registra handler para sinal específico
     * @param signal_num Número do sinal (1-64)
     * @param handler Função handler
     */
    void setHandler(int signal_num, std::function<void()> handler) {
        if (signal_num > 0 && signal_num < 64) {
            handlers_[signal_num] = std::move(handler);
        }
    }

    /**
     * @brief Envia sinal para o grupo de processos
     * @param signal_num Número do sinal
     */
    void sendSignal(int signal_num) {
        if (signal_num > 0 && signal_num < 64) {
            pending_signals_.set(signal_num);
        }
    }

    /**
     * @brief Compartilha handlers com thread (CLONE_SIGHAND)
     */
    void share() { count_++; }

    /**
     * @brief Remove referência aos handlers
     */
    bool release() { return --count_ == 0; }
};

/**
 * @brief Contexto de `CPU` simulado (thread_struct)
 */
struct ThreadContext {
    uint64_t sp;              ///< Stack pointer
    uint64_t pc;              ///< Program counter
    std::array<uint64_t, 16> registers; ///< Registradores gerais
    bool fpu_used;            ///< Se FPU foi utilizada
    
    ThreadContext() : sp(0x7fff800000), pc(0x400000), fpu_used(false) {
        registers.fill(0);
    }
};

/**
 * @brief Informações de escalonamento EEVDF
 */
struct SchedInfo {
    int prio;                 ///< Prioridade dinâmica (0-139)
    int static_prio;          ///< Prioridade estática (100-139)
    int nice;                 ///< Valor nice (-20 a +19)
    uint64_t vruntime;        ///< Tempo virtual de execução
    uint64_t deadline;        ///< Virtual deadline (EEVDF)
    uint64_t slice;           ///< Fatia de tempo alocada
    int64_t vlag;             ///< Lag virtual para fairness
    
    SchedInfo() : prio(120), static_prio(120), nice(0), 
                  vruntime(0), deadline(0), slice(1000000), vlag(0) {}
};

/**
 * @brief Simulação completa da task_struct
 */
class Task {
private:
    static std::atomic<int> next_pid_;        ///< Gerador de PIDs
    
public:
    // Identificadores fundamentais
    int pid;                                  ///< Process ID
    int tgid;                                ///< Thread Group ID
    int ppid;                                ///< Parent Process ID
    
    // Estado atual
    ProcessState state;                       ///< Estado do processo
    
    // Estruturas auxiliares
    std::shared_ptr<MemoryManager> mm;        ///< Gerenciamento de memória
    std::shared_ptr<FileDescriptors> files;   ///< Tabela de file descriptors
    std::shared_ptr<SignalManager> signal;    ///< Gerenciamento de sinais
    
    // Contexto de `CPU` e escalonamento
    ThreadContext thread;                     ///< Contexto da CPU
    SchedInfo sched_info;                    ///< Informações de escalonamento
    
    // Hierarquia de processos
    std::vector<std::shared_ptr<Task>> children; ///< Processos filhos
    std::weak_ptr<Task> parent;              ///< Processo pai

    /**
     * @brief Construtor criando nova task
     * @param is_thread Se é thread (compartilha TGID)
     * @param parent_task Processo pai
     */
    Task(bool is_thread = false, std::shared_ptr<Task> parent_task = nullptr) 
        : pid(next_pid_++), state(ProcessState::TASK_RUNNING) {
        
        if (parent_task) {
            ppid = parent_task->pid;
            tgid = is_thread ? parent_task->tgid : pid;
            parent = parent_task;
            parent_task->children.push_back(shared_from_this());
        } else {
            ppid = 0;  // Init process
            tgid = pid;
        }
        
        // Inicializa estruturas auxiliares
        mm = std::make_shared<MemoryManager>();
        files = std::make_shared<FileDescriptors>();
        signal = std::make_shared<SignalManager>();
    }

    /**
     * @brief Calcula virtual deadline usando fórmula EEVDF
     * @return Virtual deadline calculado
     */
    uint64_t calculateVirtualDeadline() const {
        // Virtual Deadline = Eligible Time + (Time Slice / Weight)
        uint64_t weight = 1024 >> (sched_info.nice + 20); // Peso baseado em nice
        return sched_info.vruntime + (sched_info.slice / weight);
    }

    /**
     * @brief Atualiza informações de escalonamento
     * @param runtime Tempo de execução em nanossegundos
     */
    void updateScheduling(uint64_t runtime) {
        sched_info.vruntime += runtime;
        sched_info.deadline = calculateVirtualDeadline();
        
        // Atualiza lag baseado na média do sistema (simplificado)
        sched_info.vlag = static_cast<int64_t>(runtime) - 1000000; // Mock calculation
    }

    /**
     * @brief Obtém estatísticas completas da task
     */
    auto getTaskStats() const {
        auto [mm_users, mm_count] = mm->getCounters();
        auto [files_count, open_fds, close_exec] = files->getStats();
        
        return std::make_tuple(pid, tgid, ppid, static_cast<int>(state),
                              mm_users, mm_count, files_count, open_fds);
    }
};

std::atomic<int> Task::next_pid_{1};

/**
 * @brief Escalonador EEVDF simplificado
 */
class EEVDFScheduler {
private:
    std::vector<std::shared_ptr<Task>> runqueue_;  ///< Fila de processos prontos
    std::shared_ptr<Task> current_task_;           ///< Task atual executando
    uint64_t system_vruntime_;                    ///< Tempo virtual do sistema

public:
    /**
     * @brief Adiciona task à fila de escalonamento
     */
    void enqueuTask(std::shared_ptr<Task> task) {
        if (task->state == ProcessState::TASK_RUNNING) {
            runqueue_.push_back(task);
        }
    }

    /**
     * @brief Seleciona próxima task usando critério EEVDF
     * @return Task com menor virtual deadline entre as elegíveis
     */
    std::shared_ptr<Task> pickNextTask() {
        if (runqueue_.empty()) return nullptr;

        // Filtra tasks elegíveis (vlag >= 0) e seleciona menor deadline
        auto eligible_tasks = runqueue_ 
                            | std::views::filter([](const auto& task) {
                                return task->sched_info.vlag >= 0;
                              });

        if (std::ranges::empty(eligible_tasks)) {
            // Se nenhuma task elegível, seleciona menor vruntime
            return *std::ranges::min_element(runqueue_, 
                [](const auto& a, const auto& b) {
                    return a->sched_info.vruntime < b->sched_info.vruntime;
                });
        }

        return *std::ranges::min_element(eligible_tasks,
            [](const auto& a, const auto& b) {
                return a->calculateVirtualDeadline() < b->calculateVirtualDeadline();
            });
    }

    /**
     * @brief Simula execução por um quantum de tempo
     * @param quantum Tempo de execução em nanossegundos
     */
    void schedule(uint64_t quantum = 1000000) {
        auto next_task = pickNextTask();
        if (!next_task) return;

        // Remove da runqueue e define como atual
        std::erase(runqueue_, next_task);
        current_task_ = next_task;

        // Simula execução
        next_task->updateScheduling(quantum);
        system_vruntime_ += quantum;

        std::cout << std::format("Executing PID {} (TGID {}) - vruntime: {}, deadline: {}, lag: {}\n",
                                next_task->pid, next_task->tgid, 
                                next_task->sched_info.vruntime,
                                next_task->calculateVirtualDeadline(),
                                next_task->sched_info.vlag);

        // Recoloca na runqueue se ainda executável
        if (next_task->state == ProcessState::TASK_RUNNING) {
            runqueue_.push_back(next_task);
        }
    }

    /**
     * @brief Obtém estatísticas do escalonador
     */
    auto getStats() const {
        return std::make_tuple(runqueue_.size(), system_vruntime_,
                              current_task_ ? current_task_->pid : -1);
    }
};

/**
 * @brief Simulador principal do kernel
 */
class KernelSimulator {
private:
    std::unordered_map<int, std::shared_ptr<Task>> task_table_; ///< Tabela de processos
    EEVDFScheduler scheduler_;                                  ///< Escalonador
    std::mt19937 generator_;                                   ///< Gerador aleatório

public:
    /**
     * @brief Construtor inicializando processo init
     */
    KernelSimulator() : generator_(std::random_device{}()) {
        // Cria processo init (PID 1)
        auto init_task = std::make_shared<Task>();
        task_table_[init_task->pid] = init_task;
        scheduler_.enqueuTask(init_task);
    }

    /**
     * @brief Simula fork() - cria novo processo
     * @param parent_pid PID do processo pai
     * @return PID do novo processo ou erro
     */
    std::expected<int, std::string> fork(int parent_pid) {
        auto parent = task_table_.find(parent_pid);
        if (parent == task_table_.end()) {
            return std::unexpected("Parent process not found");
        }

        auto child = std::make_shared<Task>(false, parent->second);
        
        // Child herda estruturas do pai (novas instâncias)
        child->mm = std::make_shared<MemoryManager>(*parent->second->mm);
        child->files = std::make_shared<FileDescriptors>(*parent->second->files);
        child->signal = std::make_shared<SignalManager>(*parent->second->signal);

        task_table_[child->pid] = child;
        scheduler_.enqueuTask(child);

        return child->pid;
    }

    /**
     * @brief Simula clone() com flags específicas
     * @param parent_pid PID do processo pai
     * @param flags Flags determinando compartilhamento
     * @return PID da nova task ou erro
     */
    std::expected<int, std::string> clone(int parent_pid, uint32_t flags) {
        auto parent = task_table_.find(parent_pid);
        if (parent == task_table_.end()) {
            return std::unexpected("Parent process not found");
        }

        bool is_thread = flags & static_cast<uint32_t>(CloneFlags::CLONE_THREAD);
        auto child = std::make_shared<Task>(is_thread, parent->second);

        // Compartilhamento baseado em flags
        if (flags & static_cast<uint32_t>(CloneFlags::CLONE_VM)) {
            child->mm = parent->second->mm;
            child->mm->addUser();
        }
        
        if (flags & static_cast<uint32_t>(CloneFlags::CLONE_FILES)) {
            child->files = parent->second->files;
            child->files->share();
        }
        
        if (flags & static_cast<uint32_t>(CloneFlags::CLONE_SIGHAND)) {
            child->signal = parent->second->signal;
            child->signal->share();
        }

        task_table_[child->pid] = child;
        scheduler_.enqueuTask(child);

        return child->pid;
    }

    /**
     * @brief Simula exit() - termina processo
     * @param pid PID do processo a terminar
     */
    void exit(int pid) {
        auto task = task_table_.find(pid);
        if (task == task_table_.end()) return;

        task->second->state = ProcessState::TASK_ZOMBIE;
        
        // Libera recursos compartilhados
        if (task->second->mm->removeUser()) {
            // Último usuário do mm_struct
        }
        
        if (task->second->files->release()) {
            // Última referência aos file descriptors
        }
        
        if (task->second->signal->release()) {
            // Última referência aos signal handlers
        }

        std::cout << std::format("Process {} exited, state: ZOMBIE\n", pid);
    }

    /**
     * @brief Executa simulação por N ciclos
     * @param cycles Número de ciclos a simular
     */
    void runSimulation(int cycles = 10) {
        std::cout << "\n=== Simulação de Gerenciamento de Processos ===\n\n";

        for (int i = 0; i < cycles; ++i) {
            std::cout << std::format("--- Ciclo {} ---\n", i + 1);
            
            // Operações aleatórias: fork, clone, exit
            std::uniform_int_distribution<> op_dist(0, 2);
            std::uniform_int_distribution<> pid_dist(1, std::max(1, static_cast<int>(task_table_.size())));
            
            switch (op_dist(generator_)) {
            case 0: // fork
                if (auto result = fork(pid_dist(generator_))) {
                    std::cout << std::format("fork() -> PID {}\n", *result);
                }
                break;
            case 1: // clone (thread)
                if (auto result = clone(pid_dist(generator_), 
                                      static_cast<uint32_t>(CloneFlags::CLONE_VM) |
                                      static_cast<uint32_t>(CloneFlags::CLONE_FILES) |
                                      static_cast<uint32_t>(CloneFlags::CLONE_THREAD))) {
                    std::cout << std::format("clone() -> TID {}\n", *result);
                }
                break;
            case 2: // exit
                if (task_table_.size() > 1) { // Manter pelo menos init
                    exit(pid_dist(generator_));
                }
                break;
            }

            // Executa escalonador
            scheduler_.schedule();
            
            auto [runqueue_size, sys_vruntime, current_pid] = scheduler_.getStats();
            std::cout << std::format("Scheduler: {} tasks, sys_vruntime: {}, current: {}\n\n",
                                    runqueue_size, sys_vruntime, current_pid);
        }
    }

    /**
     * @brief Exibe estado atual do sistema
     */
    void printSystemState() {
        std::cout << "\n=== Estado do Sistema ===\n";
        std::cout << std::format("Total de processos/threads: {}\n", task_table_.size());
        
        for (const auto& [pid, task] : task_table_) {
            auto [task_pid, tgid, ppid, state, mm_users, mm_count, files_count, open_fds] = task->getTaskStats();
            std::cout << std::format("PID: {} TGID: {} PPID: {} State: {} MM: {}/{} Files: {}/{}\n",
                                    task_pid, tgid, ppid, state, mm_users, mm_count, files_count, open_fds);
        }
    }
};

/**
 * @brief Função principal demonstrando o simulador
 */
int main() {
    std::cout << "=== Simulador de Gerenciamento de Processos **Linux** - C++23 ===\n";
    std::cout << "Implementação da task_struct, mm_struct, files_struct e escalonador EEVDF\n";

    KernelSimulator kernel;
    
    // Estado inicial
    `kernel`.printSystemState();
    
    // Executa simulação
    `kernel`.runSimulation(15);
    
    // Estado final
    `kernel`.printSystemState();

    std::cout << "\n💡 Conceitos Demonstrados:\n";
    std::cout << "• Hierarquia de processos (PID, TGID, PPID)\n";
    std::cout << "• Compartilhamento via CLONE_* flags\n";
    std::cout << "• Escalonamento EEVDF com virtual deadlines\n";
    std::cout << "• Gerenciamento de recursos (mm_struct, files_struct)\n";
    std::cout << "• Estados de processo e transições\n";
    std::cout << "• Contadores de referência para estruturas compartilhadas\n";

    return 0;
}
```

:::

O simulador de **gerenciamento de processos** implementado no código @lst-simul2 oferece uma representação funcional dos mecanismos fundamentais do `kernel` **Linux** através de classes C++23 que modelam as estruturas de dados críticas. A classe `Task` encapsula a `task_struct` com todos os campos essenciais: identificadores (`pid`, `tgid`, `ppid`), estado atual, e ponteiros compartilhados para `MemoryManager` (simulando `mm_struct`), `FileDescriptors` (simulando `files_struct`) e `SignalManager` (simulando `signal_struct`). O construtor implementa a lógica de criação de processos e threads, determinando se a nova task compartilha o Thread Group ID baseado no parâmetro `is_thread`, e estabelecendo corretamente a hierarquia pai-filho através de `std::weak_ptr` para evitar referências circulares.

A implementação do escalonador **EEVDF** na classe `EEVDFScheduler` demonstra os conceitos fundamentais do algoritmo através do método `pickNextTask()`, que primeiro filtra tasks elegíveis baseado em `vlag >= 0` usando C++23 ranges, e então seleciona a task com menor virtual deadline calculado pela fórmula `vruntime + (slice/weight)`. O método `updateScheduling()` simula a atualização de métricas após execução, incrementando `vruntime` e recalculando o deadline, enquanto a lógica de `clone()` implementa fielmente o comportamento dos `CLONE_*` flags: `CLONE_VM` compartilha o espaço de endereçamento incrementando contadores da `MemoryManager`, `CLONE_FILES` compartilha a tabela de file descriptors, e `CLONE_SIGHAND` compartilha handlers de sinais. Esta abordagem permite visualizar concretamente como threads diferem de processos através do compartilhamento seletivo de recursos, validando os conceitos teóricos através de simulação prática com contadores atômicos e estruturas de dados que refletem o comportamento real do `kernel`.
