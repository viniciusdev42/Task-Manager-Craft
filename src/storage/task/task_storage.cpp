#include "task_storage.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

const char* const kHeader = "#PERSISTENT_TASK v1";
constexpr char kSep = '\t';
constexpr std::size_t kFieldCount = 7;

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\t': out += "\\t";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            default:   out += c;
        }
    }
    return out;
}

// Retorna false se houver uma sequência de escape inválida.
bool unescape(const std::string& s, std::string& out) {
    out.clear();
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '\\') {
            out += s[i];
            continue;
        }
        if (++i >= s.size()) return false;
        switch (s[i]) {
            case '\\': out += '\\'; break;
            case 't':  out += '\t'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            default:   return false;
        }
    }
    return true;
}

// Divide preservando campos vazios (ex.: descrição vazia no fim da linha).
std::vector<std::string> split(const std::string& line, char sep) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : line) {
        if (c == sep) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

bool parseInteger(const std::string& s, long long& out) {
    if (s.empty()) return false;
    try {
        std::size_t pos = 0;
        out = std::stoll(s, &pos);
        return pos == s.size();
    } catch (const std::exception&) {
        return false;
    }
}

}  // namespace

std::string serialize(const Task& task) {
    std::string line;
    line += std::to_string(task.id);
    line += kSep;
    line += std::to_string(static_cast<int>(task.priority));
    line += kSep;
    line += std::to_string(static_cast<int>(task.status));
    line += kSep;
    line += std::to_string(static_cast<long long>(task.deadline));
    line += kSep;
    line += std::to_string(static_cast<long long>(task.timeStamp));
    line += kSep;
    line += escape(task.title);
    line += kSep;
    line += escape(task.description);
    return line;
}

std::optional<Task> deserialize(const std::string& line) {
    const auto fields = split(line, kSep);
    if (fields.size() != kFieldCount) return std::nullopt;

    long long id, priority, status, deadline, timeStamp;
    if (!parseInteger(fields[0], id) ||
        !parseInteger(fields[1], priority) ||
        !parseInteger(fields[2], status) ||
        !parseInteger(fields[3], deadline) ||
        !parseInteger(fields[4], timeStamp)) {
        return std::nullopt;
    }

    if (id < 0 || id > 2147483647LL) return std::nullopt;
    if (priority < static_cast<int>(Priority::Low) ||
        priority > static_cast<int>(Priority::Urgent)) return std::nullopt;
    if (status < static_cast<int>(Status::Pending) ||
        status > static_cast<int>(Status::Cancelled)) return std::nullopt;

    std::string title, description;
    if (!unescape(fields[5], title) || !unescape(fields[6], description)) {
        return std::nullopt;
    }

    Task task(static_cast<int>(id), std::move(title), std::move(description),
              static_cast<Priority>(priority),
              static_cast<std::time_t>(deadline));
    // O construtor define status = Pending e timeStamp = agora;
    // restauramos os valores originais gravados em disco.
    task.status = static_cast<Status>(status);
    task.timeStamp = static_cast<std::time_t>(timeStamp);
    return task;
}

bool saveTasks(const std::string& path, const std::vector<Task>& tasks) {
    const std::string tmpPath = path + ".tmp";

    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) return false;

        out << kHeader << '\n';
        for (const Task& t : tasks) {
            out << serialize(t) << '\n';
        }
        out.flush();
        if (!out) {
            out.close();
            std::error_code ignored;
            std::filesystem::remove(tmpPath, ignored);
            return false;
        }
    }  // fecha o arquivo antes do rename

    std::error_code ec;
    std::filesystem::rename(tmpPath, path, ec);
    if (ec) {
        std::filesystem::remove(tmpPath, ec);
        return false;
    }
    return true;
}

LoadResult loadTasks(const std::string& path) {
    LoadResult result;

    std::ifstream in(path, std::ios::binary);
    if (!in) return result;  // arquivo inexistente: primeira execução
    result.fileExisted = true;

    std::string line;
    if (!std::getline(in, line)) return result;  // arquivo vazio

    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != kHeader) {
        result.headerValid = false;
        return result;
    }

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (auto task = deserialize(line)) {
            result.tasks.push_back(std::move(*task));
        } else {
            ++result.skippedLines;
        }
    }
    return result;
}
