#ifndef TASK_SERIALIZER_HEADER
#define TASK_SERIALIZER_HEADER

#include "task.hpp"

#include <iosfwd>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Modulo de serializacao: converte Task <-> texto.
// Nao conhece arquivos: trabalha apenas com std::string e streams.
//
// Formato (versao 1), texto UTF-8, uma tarefa por linha:
//
//   TASKS v1
//   id|titulo|descricao|prioridade|prazo|status|criadaEm
//
// - prioridade e status sao gravados como inteiros (valor do enum);
// - prazo e criadaEm sao gravados como segundos desde a epoch;
// - nos campos de texto, '\', '|', '\n' e '\r' sao escapados com '\'.
namespace serializer {

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Converte uma tarefa em uma unica linha (sem '\n' final).
std::string serialize(const Task& task);

// Converte uma linha em tarefa. Retorna std::nullopt se a linha for invalida
// (campos faltando/sobrando, numero malformado, enum fora do intervalo,
// escape desconhecido).
std::optional<Task> deserialize(const std::string& line);

// Escreve cabecalho + todas as tarefas no stream.
void writeAll(std::ostream& out, const std::vector<Task>& tasks);

// Le cabecalho + tarefas do stream. Stream vazio resulta em vetor vazio.
// Lanca ParseError (com o numero da linha) se o conteudo for invalido.
std::vector<Task> readAll(std::istream& in);

}  
#endif 
