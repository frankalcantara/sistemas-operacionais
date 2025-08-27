# Sistema Producer-Consumer: Calculadora de Números Primos

## Objetivo do Projeto

Implementar um sistema multi-threaded usando o padrão Producer-Consumer para calcular números primos em intervalos definidos, demonstrando conceitos de sincronização de _threads_, _buffer_s compartilhados e balanceamento de carga computacional.

**Duração:** 4 horas.  
**Complexidade:** Intermediária.
**Linguagem:** C++20 ou superior, ou C, ou Rust, ou Python (Cuidado com a Versão do Python).

## Visão Geral

O sistema será composto por:

- **_threads_ Produtoras**: Geram intervalos de números para verificação de primalidade;
- **_threads_ Consumidoras**: Testam se os números nos intervalos são primos;
- **_buffer_ Compartilhado**: Fila thread-safe de intervalos de trabalho;
- **Thread Principal**: Coleta resultados e exibe estatísticas.

## Especificações Técnicas

**Nota**: Você deve criar um repositório no Github, ou uma pasta em um repositório existente. Armazene todo o desenvolvimento neste repositório.

### Estrutura de Dados

```cpp
struct Interval {
    int start;
    int end;
    int id;  // identificador do intervalo
};

struct PrimeResult {
    int number;
    bool is_prime;
    int interval_id;
    std::chrono::duration<double> calculation_time;
};
```

### Configuração do Sistema

- **Produtores**: 2 _threads_;
- **Consumidores**: 3-4 _threads_ (configurável);
- **_buffer_ size**: 10 intervalos;
- **Intervalo de números**: 1.000 a 100.000.000;
- **Tamanho do intervalo**: 1000 números por intervalo.

## Requisitos Funcionais

### 1. Thread Produtora

- Dividir o range total (1.000 - 100.000.000) em intervalos de 1000 números;
- Inserir intervalos no _buffer_ compartilhado;
- Implementar rate limiting (pequeno delay entre produções);
- Parar quando todos os intervalos forem produzidos.

### 2. Thread Consumidora

- Retirar intervalos do _buffer_;
- Para cada número no intervalo, verificar se é primo usando trial division;
- Armazenar resultados em estrutura thread-safe;
- Medir tempo de processamento de cada número.

### 3. Função de Alto Custo Computacional

```cpp
bool is_prime(int n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    
    // Trial division até √n (método custoso intencionalmente)
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}
```

### 4. _buffer_ _Thread-Safe_

Você pode usar um fila para _buffer_ de dados. No C++ 20 ou maior, você pode usar `std::queue` com `std::mutex` e `std::condition_variable` para implementar um _buffer_ _thread-safe_.

- Usar `std::queue` com `std::mutex` e `std::condition_variable`;
- Implementar operações `push()` e `pop()` _thread-safe_;
- Controlar quando _buffer_ está cheio/vazio.

### 5. Sistema de Monitoramento

- Thread principal coleta resultados periodicamente;
- Exibe estatísticas em tempo real:
  - Intervalos processados;
  - Números primos encontrados;
  - Taxa de processamento (números/segundo);
  - Tempo médio por cálculo;
  - Status do _buffer_ (ocupação atual).

## Interface de Saída

O programa deve exibir no console:

```
=== CALCULADORA DE NÚMEROS PRIMOS ===
Range: 1000 - 100000 | Intervalos: 990 | Produtores: 2 | Consumidores: 4

[14:23:45] _buffer_: [##########] 1000/1000 (100%)
[14:23:45] Processados: 45 intervalos (4.5%)
[14:23:45] Primos encontrados: 432
[14:23:45] Taxa: 1,247 números/segundo
[14:23:45] Tempo médio: 0.8ms por número

Últimos primos encontrados: 98,689 | 98,711 | 98,713 | 98,717

[14:23:50] PROCESSAMENTO COMPLETO!
Total de primos no range: 8,685
Tempo total: 78.3 segundos
```

## Requisitos de Implementação

### Sincronização Obrigatória

- `std::mutex` para proteção do _buffer_;
- `std::condition_variable` para notificação entre _threads_;
- `std::atomic` para contadores compartilhados;
- Evitar race conditions e deadlocks.

### Tratamento de Shutdown

- Sinal para parar produtores sem erros;
- Consumidores processam itens restantes no _buffer_;
- Join de todas as _threads_ antes de finalizar.

### Medição de Performance

- Cronometrar tempo total de execução;
- Medir tempo médio de cálculo por número;
- Comparar performance com diferentes números de _threads_ consumidoras.

## Critérios de Avaliação

### Funcionalidade (40%)

- Sistema produz e consome corretamente;
- Cálculo de primos está correto;
- Sincronização funciona sem travamentos;
- Output apresenta informações corretas.

### Implementação (30%)

- Uso correto de _threads_ e primitivas de sincronização;
- Código bem estruturado e legível;
- Tratamento adequado de condições de corrida;
- Gerenciamento correto de recursos.

### Performance (20%)

- Sistema utiliza recursos de forma eficiente;
- _threads_ trabalham de forma balanceada;
- _buffer_ não causa gargalos desnecessários.

### Documentação (10%)

- Comentários explicando partes críticas;
- README com instruções de compilação e execução;
- Explicação das decisões de design.

## Cronograma Sugerido (4 horas)

### Hora 1: Estrutura Básica

- Definir estruturas de dados;
- Implementar _buffer_ thread-safe básico;
- Criar função `is_prime()`;
- Teste unitário da função de primalidade.

### Hora 2: _threads_ Produtoras

- Implementar lógica de produção de intervalos
- Adicionar no _buffer_ compartilhado
- Testar produção sem consumidores

### Hora 3: _threads_ Consumidoras

- Implementar lógica de consumo;
- Processar intervalos e calcular primos;
- Armazenar resultados thread-safe.

### Hora 4: Integração e Monitoramento

- Thread principal para estatísticas;
- Output formatado em tempo real;
- Testes finais e ajustes de performance.

## Dicas de Implementação

### _buffer_ Thread-Safe

```cpp
class Interval_buffer_ {
private:
    std::queue<Interval> _buffer_;
    std::mutex mtx;
    std::condition_variable cv_not_full, cv_not_empty;
    size_t max_size;
    bool shutdown = false;

public:
    bool push(const Interval& item);
    bool pop(Interval& item);
    void signal_shutdown();
};
```

### Controle de Estatísticas

```cpp
struct Statistics {
    std::atomic<int> intervals_processed{0};
    std::atomic<int> primes_found{0};
    std::atomic<long long> total_numbers_checked{0};
    std::chrono::steady_clock::time_point start_time;
};
```

### Compilação

```bash
g++ -std=c++17 -pthread -O2 -o prime_calculator main.cpp
```

## Extensões Opcionais (se sobrar tempo)

- Implementar diferentes algoritmos de teste de primalidade;
- Adicionar opção de configurar range e número de _threads_ via linha de comando;
- Salvar resultados em arquivo;
- Comparar performance entre diferentes configurações;
- Implementar pattern Thread Pool ao invés de _threads_ fixas.

## Entregáveis

1. Código fonte completo (`main.cpp` e headers se necessário);
2. Makefile ou script de compilação;
3. README.md com instruções e análise de resultados;
4. Teste demonstrando o funcionamento com screenshot/log da saída.
