#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <limits>

using namespace std;
using namespace std::chrono;

// Estrutura para armazenar os dados do CSV
struct DataSet {
    vector<int> lista_aleatoria;
    vector<int> lista_80_ordenada;
    vector<int> lista_decrescente;
};

// Estrutura para armazenar resultados de tempo
struct ResultadoTempo {
    double tempo_aleatorio;
    double tempo_80_ordenado;
    double tempo_decrescente;
};

// ==================== PROTÓTIPOS DAS FUNÇÕES ====================
DataSet lerCSV(const string& nomeArquivo);
void bubbleSort(vector<int>& arr);
void insertionSort(vector<int>& arr);
void selectionSort(vector<int>& arr);
void mergeSort(vector<int>& arr);
void mergeSortRecursivo(vector<int>& arr, int esquerda, int direita);
void merge(vector<int>& arr, int esquerda, int meio, int direita);
template<typename Func> double medirTempo(vector<int> dados, Func algoritmo);
bool estaOrdenado(const vector<int>& arr);
void testarAlgoritmos(const DataSet& dados);
void exibirTabelaResultados(const vector<pair<string, ResultadoTempo>>& resultados);
void analisarResultados(const vector<pair<string, ResultadoTempo>>& resultados);

// Função para ler o arquivo CSV
DataSet lerCSV(const string& nomeArquivo) {
    DataSet dados;
    ifstream arquivo(nomeArquivo);
    string linha;

    if (!arquivo.is_open()) {
        cerr << "Erro: Não foi possível abrir o arquivo " << nomeArquivo << endl;
        return dados;
    }

    // Pular o cabeçalho
    getline(arquivo, linha);

    // Ler os dados
    while (getline(arquivo, linha)) {
        stringstream ss(linha);
        string valor;

        // Ler Lista_Aleatoria
        if (getline(ss, valor, ',')) {
            dados.lista_aleatoria.push_back(stoi(valor));
        }

        // Ler Lista_80_Ordenada
        if (getline(ss, valor, ',')) {
            dados.lista_80_ordenada.push_back(stoi(valor));
        }

        // Ler Lista_Decrescente
        if (getline(ss, valor, ',')) {
            dados.lista_decrescente.push_back(stoi(valor));
        }
    }

    arquivo.close();
    cout << "Arquivo CSV carregado com sucesso!" << endl;
    cout << "Dados carregados: " << dados.lista_aleatoria.size() << " elementos por lista" << endl;
    return dados;
}

// ==================== ALGORITMOS DE ORDENAÇÃO ====================

// Bubble Sort
void bubbleSort(vector<int>& arr) {
    int n = arr.size();
    for (int i = 0; i < n - 1; i++) {
        bool trocou = false;
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(arr[j], arr[j + 1]);
                trocou = true;
            }
        }
        // Otimização: se não houve trocas, o array já está ordenado
        if (!trocou) break;
    }
}

// Insertion Sort
void insertionSort(vector<int>& arr) {
    int n = arr.size();
    for (int i = 1; i < n; i++) {
        int chave = arr[i];
        int j = i - 1;

        // Move elementos maiores que a chave uma posição à frente
        while (j >= 0 && arr[j] > chave) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = chave;
    }
}

// Selection Sort
void selectionSort(vector<int>& arr) {
    int n = arr.size();
    for (int i = 0; i < n - 1; i++) {
        int minIdx = i;

        // Encontra o menor elemento no array não ordenado
        for (int j = i + 1; j < n; j++) {
            if (arr[j] < arr[minIdx]) {
                minIdx = j;
            }
        }

        // Troca o menor elemento encontrado com o primeiro elemento
        if (minIdx != i) {
            swap(arr[i], arr[minIdx]);
        }
    }
}

// Merge Sort - Função auxiliar para merge
void merge(vector<int>& arr, int esquerda, int meio, int direita) {
    int n1 = meio - esquerda + 1;
    int n2 = direita - meio;

    // Arrays temporários
    vector<int> L(n1), R(n2);

    // Copia dados para arrays temporários
    for (int i = 0; i < n1; i++)
        L[i] = arr[esquerda + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[meio + 1 + j];

    // Merge os arrays temporários de volta em arr[esquerda..direita]
    int i = 0, j = 0, k = esquerda;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    // Copia os elementos restantes de L[], se houver
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    // Copia os elementos restantes de R[], se houver
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

// Merge Sort - Função principal
void mergeSortRecursivo(vector<int>& arr, int esquerda, int direita) {
    if (esquerda < direita) {
        int meio = esquerda + (direita - esquerda) / 2;

        // Ordena primeira e segunda metades
        mergeSortRecursivo(arr, esquerda, meio);
        mergeSortRecursivo(arr, meio + 1, direita);

        // Merge as duas metades ordenadas
        merge(arr, esquerda, meio, direita);
    }
}

// Wrapper para Merge Sort
void mergeSort(vector<int>& arr) {
    if (arr.size() > 1) {
        mergeSortRecursivo(arr, 0, arr.size() - 1);
    }
}

// ==================== FUNÇÕES DE TESTE E MEDIÇÃO ====================

// Função para medir tempo de execução de um algoritmo
template<typename Func>
double medirTempo(vector<int> dados, Func algoritmo) {
    auto inicio = high_resolution_clock::now();
    algoritmo(dados);
    auto fim = high_resolution_clock::now();

    auto duracao = duration_cast<microseconds>(fim - inicio);
    return duracao.count() / 1000.0; // Retorna em milissegundos
}

// Função para verificar se um array está ordenado
bool estaOrdenado(const vector<int>& arr) {
    for (size_t i = 1; i < arr.size(); i++) {
        if (arr[i] < arr[i - 1]) {
            return false;
        }
    }
    return true;
}

// Função para testar todos os algoritmos
void testarAlgoritmos(const DataSet& dados) {
    cout << "\n" << string(80, '=') << endl;
    cout << "INICIANDO TESTES DE ALGORITMOS DE ORDENAÇÃO" << endl;
    cout << string(80, '=') << endl;

    // Armazenar resultados
    vector<pair<string, ResultadoTempo>> resultados;

    // Teste Bubble Sort
    cout << "\nTestando Bubble Sort..." << endl;
    ResultadoTempo tempos_bubble;
    tempos_bubble.tempo_aleatorio = medirTempo(dados.lista_aleatoria, bubbleSort);
    tempos_bubble.tempo_80_ordenado = medirTempo(dados.lista_80_ordenada, bubbleSort);
    tempos_bubble.tempo_decrescente = medirTempo(dados.lista_decrescente, bubbleSort);
    resultados.push_back({ "Bubble Sort", tempos_bubble });

    // Teste Insertion Sort
    cout << "Testando Insertion Sort..." << endl;
    ResultadoTempo tempos_insertion;
    tempos_insertion.tempo_aleatorio = medirTempo(dados.lista_aleatoria, insertionSort);
    tempos_insertion.tempo_80_ordenado = medirTempo(dados.lista_80_ordenada, insertionSort);
    tempos_insertion.tempo_decrescente = medirTempo(dados.lista_decrescente, insertionSort);
    resultados.push_back({ "Insertion Sort", tempos_insertion });

    // Teste Selection Sort
    cout << "Testando Selection Sort..." << endl;
    ResultadoTempo tempos_selection;
    tempos_selection.tempo_aleatorio = medirTempo(dados.lista_aleatoria, selectionSort);
    tempos_selection.tempo_80_ordenado = medirTempo(dados.lista_80_ordenada, selectionSort);
    tempos_selection.tempo_decrescente = medirTempo(dados.lista_decrescente, selectionSort);
    resultados.push_back({ "Selection Sort", tempos_selection });

    // Teste Merge Sort
    cout << "Testando Merge Sort..." << endl;
    ResultadoTempo tempos_merge;
    tempos_merge.tempo_aleatorio = medirTempo(dados.lista_aleatoria, mergeSort);
    tempos_merge.tempo_80_ordenado = medirTempo(dados.lista_80_ordenada, mergeSort);
    tempos_merge.tempo_decrescente = medirTempo(dados.lista_decrescente, mergeSort);
    resultados.push_back({ "Merge Sort", tempos_merge });

    // Verificação de integridade (teste com uma pequena amostra)
    cout << "\nVerificando integridade dos algoritmos..." << endl;
    vector<int> teste = { 64, 34, 25, 12, 22, 11, 90 };
    vector<int> copia_teste;

    copia_teste = teste; bubbleSort(copia_teste);
    cout << "Bubble Sort: " << (estaOrdenado(copia_teste) ? "✓ OK" : "✗ ERRO") << endl;

    copia_teste = teste; insertionSort(copia_teste);
    cout << "Insertion Sort: " << (estaOrdenado(copia_teste) ? "✓ OK" : "✗ ERRO") << endl;

    copia_teste = teste; selectionSort(copia_teste);
    cout << "Selection Sort: " << (estaOrdenado(copia_teste) ? "✓ OK" : "✗ ERRO") << endl;

    copia_teste = teste; mergeSort(copia_teste);
    cout << "Merge Sort: " << (estaOrdenado(copia_teste) ? "✓ OK" : "✗ ERRO") << endl;

    // Exibir tabela de resultados
    exibirTabelaResultados(resultados);
}

// Função para exibir tabela formatada de resultados
void exibirTabelaResultados(const vector<pair<string, ResultadoTempo>>& resultados) {
    cout << "\n" << string(100, '=') << endl;
    cout << "TABELA DE COMPARAÇÃO DE TEMPOS DE EXECUÇÃO (em milissegundos)" << endl;
    cout << string(100, '=') << endl;

    // Cabeçalho
    cout << left << setw(20) << "ALGORITMO"
        << right << setw(20) << "LISTA ALEATÓRIA"
        << setw(20) << "LISTA 80% ORD."
        << setw(20) << "LISTA DECRESCENTE"
        << setw(20) << "MÉDIA" << endl;
    cout << string(100, '-') << endl;

    // Dados
    for (const auto& resultado : resultados) {
        double media = (resultado.second.tempo_aleatorio +
            resultado.second.tempo_80_ordenado +
            resultado.second.tempo_decrescente) / 3.0;

        cout << left << setw(20) << resultado.first
            << right << setw(20) << fixed << setprecision(2) << resultado.second.tempo_aleatorio
            << setw(20) << fixed << setprecision(2) << resultado.second.tempo_80_ordenado
            << setw(20) << fixed << setprecision(2) << resultado.second.tempo_decrescente
            << setw(20) << fixed << setprecision(2) << media << endl;
    }

    cout << string(100, '=') << endl;

    // Análise dos resultados
    analisarResultados(resultados);
}

// Função para análise automática dos resultados
void analisarResultados(const vector<pair<string, ResultadoTempo>>& resultados) {
    cout << "\n" << string(80, '=') << endl;
    cout << "ANÁLISE DOS RESULTADOS" << endl;
    cout << string(80, '=') << endl;

    // Encontrar o mais rápido para cada tipo de lista
    string mais_rapido_aleatorio, mais_rapido_80, mais_rapido_decrescente;
    double menor_tempo_aleatorio = numeric_limits<double>::max();
    double menor_tempo_80 = numeric_limits<double>::max();
    double menor_tempo_decrescente = numeric_limits<double>::max();

    for (const auto& resultado : resultados) {
        if (resultado.second.tempo_aleatorio < menor_tempo_aleatorio) {
            menor_tempo_aleatorio = resultado.second.tempo_aleatorio;
            mais_rapido_aleatorio = resultado.first;
        }
        if (resultado.second.tempo_80_ordenado < menor_tempo_80) {
            menor_tempo_80 = resultado.second.tempo_80_ordenado;
            mais_rapido_80 = resultado.first;
        }
        if (resultado.second.tempo_decrescente < menor_tempo_decrescente) {
            menor_tempo_decrescente = resultado.second.tempo_decrescente;
            mais_rapido_decrescente = resultado.first;
        }
    }

    cout << "🏆 MELHOR DESEMPENHO POR CATEGORIA:" << endl;
    cout << "   • Lista Aleatória: " << mais_rapido_aleatorio
        << " (" << fixed << setprecision(2) << menor_tempo_aleatorio << "ms)" << endl;
    cout << "   • Lista 80% Ordenada: " << mais_rapido_80
        << " (" << fixed << setprecision(2) << menor_tempo_80 << "ms)" << endl;
    cout << "   • Lista Decrescente: " << mais_rapido_decrescente
        << " (" << fixed << setprecision(2) << menor_tempo_decrescente << "ms)" << endl;

    cout << "\n📊 OBSERVAÇÕES:" << endl;
    cout << "   • Merge Sort geralmente tem melhor desempenho geral (O(n log n))" << endl;
    cout << "   • Insertion Sort é eficiente para listas quase ordenadas" << endl;
    cout << "   • Bubble Sort é o menos eficiente para listas grandes" << endl;
    cout << "   • Selection Sort tem desempenho consistente mas não otimizado" << endl;

    cout << "\n💡 DICA PEDAGÓGICA:" << endl;
    cout << "   Esta diferença demonstra a importância da análise de complexidade" << endl;
    cout << "   algorítmica na escolha do algoritmo adequado para cada situação." << endl;
    cout << string(80, '=') << endl;
}

// Função principal
int main() {
    cout << string(80, '=') << endl;
    cout << "ANALISADOR DE ALGORITMOS DE ORDENAÇÃO" << endl;
    cout << "Desenvolvido para a disciplina de Raciocínio Algorítmico" << endl;
    cout << string(80, '=') << endl;

    string nomeArquivo;
    cout << "\nDigite o nome do arquivo CSV (ex: dados_algoritmos.csv): ";
    getline(cin, nomeArquivo);

    // Carregar dados do CSV
    DataSet dados = lerCSV(nomeArquivo);

    if (dados.lista_aleatoria.empty()) {
        cerr << "Erro: Não foi possível carregar os dados. Verifique o arquivo." << endl;
        return 1;
    }

    // Verificar tamanho dos dados
    cout << "\nEstatísticas dos dados carregados:" << endl;
    cout << "• Lista Aleatória: " << dados.lista_aleatoria.size() << " elementos" << endl;
    cout << "• Lista 80% Ordenada: " << dados.lista_80_ordenada.size() << " elementos" << endl;
    cout << "• Lista Decrescente: " << dados.lista_decrescente.size() << " elementos" << endl;

    // Executar testes
    testarAlgoritmos(dados);

    cout << "\nPressione Enter para finalizar...";
    cin.get();

    return 0;
}