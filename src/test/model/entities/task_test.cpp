/*
  EXECUÇÃO LOCAL 
  
  1. curl -O https://raw.githubusercontent.com/doctest/doctest/master/doctest/doctest.h
  
   2. g++ -std=c++11 task.cpp test_task.cpp -o test_task

*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "task.hpp"
#include <ctime>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

namespace {

// Executa `fn` e devolve tudo que ela escreveu em std::cout.
// O redirecionamento é desfeito antes de retornar, para que as mensagens
// do próprio doctest não sejam engolidas em caso de falha.
std::string capture(const std::function<void()>& fn) {
    std::ostringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    try {
        fn();
    } catch (...) {
        std::cout.rdbuf(old);
        throw;
    }
    std::cout.rdbuf(old);
    return buffer.str();
}

// Cria um time_t a partir de uma data local (independe do fuso horário).
std::time_t makeTime(int day, int month, int year, int hour, int min) {
    std::tm tm{};
    tm.tm_mday = day;
    tm.tm_mon = month - 1;
    tm.tm_year = year - 1900;
    tm.tm_hour = hour;
    tm.tm_min = min;
    tm.tm_isdst = -1;
    return std::mktime(&tm);
}

constexpr std::time_t kHour = 60 * 60;
constexpr std::time_t kDay = 24 * kHour;

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

Task makeTask(std::time_t deadline) {
    return Task(1, "Titulo", "Descricao", Priority::Medium, deadline);
}

}  // namespace

// ---------------------------------------------------------------------------
// forString
// ---------------------------------------------------------------------------

TEST_SUITE("forString") {

TEST_CASE("Priority: texto para cada valor") {
    CHECK(forString(Priority::Low) == "Baixa");
    CHECK(forString(Priority::Medium) == "Média");
    CHECK(forString(Priority::High) == "Alta");
    CHECK(forString(Priority::Urgent) == "Urgente");
}

TEST_CASE("Priority: valor invalido retorna Desconhecida") {
    CHECK(forString(static_cast<Priority>(999)) == "Desconhecida");
}

TEST_CASE("Status: texto para cada valor") {
    CHECK(forString(Status::Pending) == "Pendente");
    CHECK(forString(Status::InProgress) == "Em andamento");
    CHECK(forString(Status::Completed) == "Concluída");
    CHECK(forString(Status::Cancelled) == "Cancelada");
}

TEST_CASE("Status: valor invalido retorna Desconhecido") {
    CHECK(forString(static_cast<Status>(999)) == "Desconhecido");
}

}  // TEST_SUITE forString

// ---------------------------------------------------------------------------
// printDate
// ---------------------------------------------------------------------------

TEST_SUITE("printDate") {

TEST_CASE("formata como dd/mm/aaaa hh:mm") {
    const std::string out = capture([] { printDate(makeTime(25, 12, 2030, 14, 30)); });
    CHECK(out == "25/12/2030 14:30");
}

TEST_CASE("preenche com zeros a esquerda") {
    const std::string out = capture([] { printDate(makeTime(5, 3, 2031, 9, 7)); });
    CHECK(out == "05/03/2031 09:07");
}

TEST_CASE("nao adiciona quebra de linha") {
    const std::string out = capture([] { printDate(makeTime(1, 1, 2030, 0, 0)); });
    CHECK(out.find('\n') == std::string::npos);
}

}  // TEST_SUITE printDate

// ---------------------------------------------------------------------------
// Task::Task (construtor)
// ---------------------------------------------------------------------------

TEST_SUITE("Task::Task") {

TEST_CASE("inicializa os campos informados") {
    const std::time_t deadline = makeTime(31, 12, 2030, 23, 59);
    Task t(42, "Estudar C++", "Revisar testes unitários", Priority::High, deadline);

    CHECK(t.id == 42);
    CHECK(t.title == "Estudar C++");
    CHECK(t.description == "Revisar testes unitários");
    CHECK(t.priority == Priority::High);
    CHECK(t.deadline == deadline);
}

TEST_CASE("status inicial e Pending") {
    Task t = makeTask(std::time(nullptr) + kDay);
    CHECK(t.status == Status::Pending);
}

TEST_CASE("timeStamp e definido no momento da criacao") {
    const std::time_t antes = std::time(nullptr);
    Task t = makeTask(antes + kDay);
    const std::time_t depois = std::time(nullptr);

    CHECK(t.timeStamp >= antes);
    CHECK(t.timeStamp <= depois);
}

TEST_CASE("aceita titulo e descricao vazios") {
    Task t(0, "", "", Priority::Low, std::time(nullptr) + kDay);
    CHECK(t.title.empty());
    CHECK(t.description.empty());
}

TEST_CASE("aceita todas as prioridades") {
    const std::time_t d = std::time(nullptr) + kDay;
    CHECK(Task(1, "a", "b", Priority::Low, d).priority == Priority::Low);
    CHECK(Task(2, "a", "b", Priority::Medium, d).priority == Priority::Medium);
    CHECK(Task(3, "a", "b", Priority::High, d).priority == Priority::High);
    CHECK(Task(4, "a", "b", Priority::Urgent, d).priority == Priority::Urgent);
}

}  // TEST_SUITE Task::Task

// ---------------------------------------------------------------------------
// Task::isLate
// ---------------------------------------------------------------------------

TEST_SUITE("Task::isLate") {

TEST_CASE("prazo no futuro nao esta atrasada") {
    Task t = makeTask(std::time(nullptr) + kDay);
    CHECK_FALSE(t.isLate());
}

TEST_CASE("prazo vencido e Pending esta atrasada") {
    Task t = makeTask(std::time(nullptr) - kHour);
    t.status = Status::Pending;
    CHECK(t.isLate());
}

TEST_CASE("prazo vencido e InProgress esta atrasada") {
    Task t = makeTask(std::time(nullptr) - kHour);
    t.status = Status::InProgress;
    CHECK(t.isLate());
}

TEST_CASE("Completed nunca esta atrasada") {
    Task t = makeTask(std::time(nullptr) - kDay);
    t.status = Status::Completed;
    CHECK_FALSE(t.isLate());
}

TEST_CASE("Cancelled nunca esta atrasada") {
    Task t = makeTask(std::time(nullptr) - kDay);
    t.status = Status::Cancelled;
    CHECK_FALSE(t.isLate());
}

TEST_CASE("prazo no futuro com status finalizado nao esta atrasada") {
    Task t = makeTask(std::time(nullptr) + kDay);
    t.status = Status::Completed;
    CHECK_FALSE(t.isLate());
    t.status = Status::Cancelled;
    CHECK_FALSE(t.isLate());
}

TEST_CASE("mudar status para Completed remove o atraso") {
    Task t = makeTask(std::time(nullptr) - kHour);
    REQUIRE(t.isLate());
    t.status = Status::Completed;
    CHECK_FALSE(t.isLate());
}

}  // TEST_SUITE Task::isLate

// ---------------------------------------------------------------------------
// Task::show
// ---------------------------------------------------------------------------

TEST_SUITE("Task::show") {

TEST_CASE("exibe todos os campos") {
    const std::time_t deadline = makeTime(25, 12, 2030, 14, 30);
    Task t(7, "Entregar relatório", "Relatório trimestral", Priority::Urgent, deadline);
    t.status = Status::InProgress;

    const std::string out = capture([&] { t.show(); });

    CHECK(contains(out, "ID:           7\n"));
    CHECK(contains(out, "Título:       Entregar relatório\n"));
    CHECK(contains(out, "Descrição:    Relatório trimestral\n"));
    CHECK(contains(out, "Prioridade:   Urgente\n"));
    CHECK(contains(out, "Status:       Em andamento\n"));
    CHECK(contains(out, "Prazo:        25/12/2030 14:30\n"));
    CHECK(contains(out, "Criada em:    "));
    CHECK(contains(out, "Atrasada:     "));
}

TEST_CASE("Atrasada: Nao quando o prazo esta no futuro") {
    Task t = makeTask(std::time(nullptr) + kDay);
    const std::string out = capture([&] { t.show(); });
    CHECK(contains(out, "Atrasada:     Não\n"));
}

TEST_CASE("Atrasada: Sim quando o prazo venceu") {
    Task t = makeTask(std::time(nullptr) - kDay);
    const std::string out = capture([&] { t.show(); });
    CHECK(contains(out, "Atrasada:     Sim\n"));
}

TEST_CASE("Criada em usa o formato de data") {
    Task t = makeTask(std::time(nullptr) + kDay);
    t.timeStamp = makeTime(1, 2, 2030, 8, 5);
    const std::string out = capture([&] { t.show(); });
    CHECK(contains(out, "Criada em:    01/02/2030 08:05\n"));
}

TEST_CASE("linhas aparecem na ordem esperada") {
    Task t = makeTask(std::time(nullptr) + kDay);
    const std::string out = capture([&] { t.show(); });

    const auto pId = out.find("ID:");
    const auto pTitulo = out.find("Título:");
    const auto pDescricao = out.find("Descrição:");
    const auto pPrioridade = out.find("Prioridade:");
    const auto pStatus = out.find("Status:");
    const auto pPrazo = out.find("Prazo:");
    const auto pCriada = out.find("Criada em:");
    const auto pAtrasada = out.find("Atrasada:");

    REQUIRE(pId != std::string::npos);
    CHECK(pId < pTitulo);
    CHECK(pTitulo < pDescricao);
    CHECK(pDescricao < pPrioridade);
    CHECK(pPrioridade < pStatus);
    CHECK(pStatus < pPrazo);
    CHECK(pPrazo < pCriada);
    CHECK(pCriada < pAtrasada);
}

TEST_CASE("termina com quebra de linha") {
    Task t = makeTask(std::time(nullptr) + kDay);
    const std::string out = capture([&] { t.show(); });

    REQUIRE_FALSE(out.empty());
    CHECK(out.back() == '\n');
}
}  // TEST_SUITE Task::show
