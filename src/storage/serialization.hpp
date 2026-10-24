#ifndef TASK_SERIALIZER_HEADER
#define TASK_SERIALIZER_HEADER

#include "task.hpp"

#include <iosfwd>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

// Módulo de serialização: converte Task <-> texto.
// Não conhece arquivos: trabalha apenas com std::string e streams.
//
// Formato (versão 1), texto UTF-8, uma tarefa por linha:
//
//   TASKS v1
//   id|titulo|descricao|prioridade|prazo|status|criadaEm
//
// - prioridade e status são gravados como inteiros (valor do enum);
// - prazo e criadaEm são gravados como segundos desde a epoch;
// - nos campos de texto, '\', '|', '\n' e '\r' são escapados com '\'.
namespace serializer {

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Converte uma tarefa em uma única linha (sem '\n' final).
std::string serialize(const Task& task);

// Converte uma linha em tarefa. Retorna std::nullopt se a linha for inválida
// (campos faltando/sobrando, número malformado, enum fora do intervalo,
// escape desconhecido).
std::optional<Task> deserialize(const std::string& line);

// Escreve cabeçalho + todas as tarefas no stream.
void writeAll(std::ostream& out, const std::vector<Task>& tasks);

// Lê cabeçalho + tarefas do stream. Stream vazio resulta em vetor vazio.
// Lança ParseError (com o número da linha) se o conteúdo for inválido.
std::vector<Task> readAll(std::istream& in);

}  // namespace serializer

#endif // TASK_SERIALIZER_HEADER
