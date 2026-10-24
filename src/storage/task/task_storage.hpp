#ifndef TASK_STORAGE_HEADER
#define TASK_STORAGE_HEADER

#include "task.hpp"

#include <optional>
#include <string>
#include <vector>

// Converte uma tarefa em uma linha de texto (sem '\n' no final).
// Formato (campos separados por TAB):
//   id  prioridade  status  prazo  criadaEm  título  descrição
// Título e descrição são "escapados" (\\, \t, \n, \r) para que nunca
// quebrem a estrutura do arquivo.
std::string serialize(const Task& task);

// Reconstrói uma tarefa a partir de uma linha. Retorna std::nullopt
// se a linha estiver malformada ou com valores inválidos.
std::optional<Task> deserialize(const std::string& line);

// Grava todas as tarefas em `path`. A gravação é feita em um arquivo
// temporário que depois substitui o original, evitando corromper os
// dados caso o programa seja interrompido no meio da escrita.
bool saveTasks(const std::string& path, const std::vector<Task>& tasks);

struct LoadResult {
    std::vector<Task> tasks;
    bool fileExisted = false;   // false => primeira execução (sem arquivo)
    bool headerValid = true;    // false => cabeçalho ausente/incompatível
    int skippedLines = 0;       // linhas ignoradas por estarem inválidas
};

// Carrega as tarefas de `path`. Se o arquivo não existir, retorna lista vazia.
LoadResult loadTasks(const std::string& path);

#endif // TASK_STORAGE_HEADER
