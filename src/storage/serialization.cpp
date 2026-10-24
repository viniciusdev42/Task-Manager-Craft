#include "serialization.hpp"

#include <charconv>
#include <istream>
#include <limits>
#include <ostream>
#include <system_error>

namespace serializer {

namespace {

constexpr char kSeparator = '|';
constexpr char kEscape = '\\';
constexpr const char* kHeader = "TASKS v1";
constexpr std::size_t kFieldCount = 7;

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case kEscape:    out += "\\\\"; break;
            case kSeparator: out += "\\|";  break;
            case '\n':       out += "\\n";  break;
            case '\r':       out += "\\r";  break;
            default:         out += c;
        }
    }
    return out;
}

// Divide a linha nos separadores não escapados e já desfaz os escapes.
std::optional<std::vector<std::string>> splitFields(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == kEscape) {
            if (++i >= line.size()) return std::nullopt;  // '\' no fim da linha
            switch (line[i]) {
                case kEscape:    current += kEscape;    break;
                case kSeparator: current += kSeparator; break;
                case 'n':        current += '\n';       break;
                case 'r':        current += '\r';       break;
                default:         return std::nullopt;   // escape desconhecido
            }
        } else if (c == kSeparator) {
            fields.push_back(std::move(current));
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(std::move(current));
    return fields;
}

std::optional<long long> parseInteger(const std::string& s) {
    if (s.empty()) return std::nullopt;
    long long value = 0;
    const char* first = s.data();
    const char* last = first + s.size();
    auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) return std::nullopt;
    return value;
}

std::optional<Priority> parsePriority(const std::string& s) {
    auto v = parseInteger(s);
    if (!v || *v < static_cast<int>(Priority::Low) ||
        *v > static_cast<int>(Priority::Urgent)) {
        return std::nullopt;
    }
    return static_cast<Priority>(*v);
}

std::optional<Status> parseStatus(const std::string& s) {
    auto v = parseInteger(s);
    if (!v || *v < static_cast<int>(Status::Pending) ||
        *v > static_cast<int>(Status::Cancelled)) {
        return std::nullopt;
    }
    return static_cast<Status>(*v);
}

std::string toText(std::time_t t) {
    return std::to_string(static_cast<long long>(t));
}

}  // namespace

std::string serialize(const Task& task) {
    std::string line;
    line += std::to_string(task.id);
    line += kSeparator;
    line += escape(task.title);
    line += kSeparator;
    line += escape(task.description);
    line += kSeparator;
    line += std::to_string(static_cast<int>(task.priority));
    line += kSeparator;
    line += toText(task.deadline);
    line += kSeparator;
    line += std::to_string(static_cast<int>(task.status));
    line += kSeparator;
    line += toText(task.timeStamp);
    return line;
}

std::optional<Task> deserialize(const std::string& line) {
    auto fields = splitFields(line);
    if (!fields || fields->size() != kFieldCount) return std::nullopt;
    const auto& f = *fields;

    auto id        = parseInteger(f[0]);
    auto priority  = parsePriority(f[3]);
    auto deadline  = parseInteger(f[4]);
    auto status    = parseStatus(f[5]);
    auto timeStamp = parseInteger(f[6]);

    if (!id || !priority || !deadline || !status || !timeStamp) return std::nullopt;
    if (*id < std::numeric_limits<int>::min() ||
        *id > std::numeric_limits<int>::max()) {
        return std::nullopt;
    }

    // O construtor define status = Pendente e timeStamp = agora;
    // restauramos os valores originais em seguida.
    Task task(static_cast<int>(*id), f[1], f[2], *priority,
              static_cast<std::time_t>(*deadline));
    task.status = *status;
    task.timeStamp = static_cast<std::time_t>(*timeStamp);
    return task;
}

void writeAll(std::ostream& out, const std::vector<Task>& tasks) {
    out << kHeader << '\n';
    for (const Task& t : tasks) {
        out << serialize(t) << '\n';
    }
}

std::vector<Task> readAll(std::istream& in) {
    std::vector<Task> tasks;
    std::string line;
    std::size_t lineNumber = 0;

    auto nextLine = [&]() -> bool {
        if (!std::getline(in, line)) return false;
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();  // CRLF
        return true;
    };

    if (!nextLine()) return tasks;  // vazio: nenhuma tarefa

    if (line != kHeader) {
        throw ParseError("cabecalho invalido ou versao nao suportada (linha 1)");
    }

    while (nextLine()) {
        if (line.empty()) continue;
        auto task = deserialize(line);
        if (!task) {
            throw ParseError("linha " + std::to_string(lineNumber) + " invalida");
        }
        tasks.push_back(std::move(*task));
    }
    return tasks;
}

}  // namespace serializer
