#ifndef TASK_STORAGE_HEADER
#define TASK_STORAGE_HEADER

#include "task.hpp"

#include <filesystem>
#include <stdexcept>
#include <vector>

// Módulo de persistência: grava e lê a lista de tarefas em um arquivo.
// Não conhece o formato: delega toda a conversão ao módulo `serializer`.
class StorageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TaskStorage {
public:
    explicit TaskStorage(std::filesystem::path filePath);

    // Grava todas as tarefas de forma atômica: escreve em "<arquivo>.tmp" e
    // só então substitui o arquivo real, evitando corromper dados já salvos
    // se o programa cair no meio da escrita. Cria o diretório pai se preciso.
    // Lança StorageError em caso de falha.
    void save(const std::vector<Task>& tasks) const;

    // Lê todas as tarefas. Se o arquivo ainda não existe, retorna vetor vazio.
    // Lança StorageError se não for possível ler ou se o conteúdo for inválido.
    std::vector<Task> load() const;

    bool exists() const;
    const std::filesystem::path& filePath() const { return filePath_; }

private:
    std::filesystem::path filePath_;
};

#endif // TASK_STORAGE_HEADER
