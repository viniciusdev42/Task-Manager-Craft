#ifndef TASK_STORAGE_HEADER
#define TASK_STORAGE_HEADER

#include "task.hpp"

#include <filesystem>
#include <stdexcept>
#include <vector>

/* Modulo de persistencia:grava e le a lista de tarefas em um arquivo.
   Nao conhece o formato,delega toda a conversao ao modulo `serializer`.
*/

class StorageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class TaskStorage {
public:
    explicit TaskStorage(std::filesystem::path filePath);

    /** Grava todas as tarefas de forma atomica: escreve em "<arquivo>.tmp" e 
        so entao substitui o arquivo real, evitando corromper dados ja salvos 
        se o programa cair no meio da escrita. 
        
        Cria o diretorio pai se preciso.
        
        Lanca StorageError em caso de falha.
    */
    void save(const std::vector<Task>& tasks) const;

    /* Le todas as tarefas. Se o arquivo ainda nao existe, retorna vetor vazio.
    
       Lanca StorageError se nao for possivel ler ou se o conteudo for invalido.
    */

    std::vector<Task> load() const;

    bool exists() const;
    const std::filesystem::path& filePath() const { return filePath_; }

private:
    std::filesystem::path filePath_;
};

#endif 
