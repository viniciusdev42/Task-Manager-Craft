#include "file_storage.hpp"
#include "serialization.hpp"

#include <fstream>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

TaskStorage::TaskStorage(fs::path filePath) : filePath_(std::move(filePath)) {}

bool TaskStorage::exists() const {
    std::error_code ec;
    return fs::is_regular_file(filePath_, ec);
}

void TaskStorage::save(const std::vector<Task>& tasks) const {
    std::error_code ec;

    if (filePath_.has_parent_path()) {
        fs::create_directories(filePath_.parent_path(), ec);
        if (ec) {
            throw StorageError("nao foi possivel criar o diretorio '" +
                               filePath_.parent_path().string() + "': " + ec.message());
        }
    }

    fs::path tmpPath = filePath_;
    tmpPath += ".tmp";

    {
        // binary: evita traducao de '\n' -> "\r\n" no Windows
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw StorageError("nao foi possivel abrir '" + tmpPath.string() +
                               "' para escrita");
        }

        serializer::writeAll(out, tasks);
        out.flush();

        if (!out) {
            out.close();
            fs::remove(tmpPath, ec);
            throw StorageError("falha ao escrever em '" + tmpPath.string() + "'");
        }
    }  // out fechado aqui

    fs::rename(tmpPath, filePath_, ec);
    if (ec) {
        std::error_code ignored;
        fs::remove(tmpPath, ignored);
        throw StorageError("nao foi possivel substituir '" + filePath_.string() +
                           "': " + ec.message());
    }
}

std::vector<Task> TaskStorage::load() const {
    if (!exists()) return {};

    std::ifstream in(filePath_, std::ios::binary);
    if (!in) {
        throw StorageError("nao foi possivel abrir '" + filePath_.string() +
                           "' para leitura");
    }

    try {
        auto tasks = serializer::readAll(in);
        if (in.bad()) {
            throw StorageError("erro de leitura em '" + filePath_.string() + "'");
        }
        return tasks;
    } catch (const serializer::ParseError& e) {
        throw StorageError("arquivo '" + filePath_.string() +
                           "' corrompido: " + e.what());
    }
}
