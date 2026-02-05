#pragma once
#include "SyslogProto.hxx"
#include <vector>
#include <string>

struct sqlite3;

namespace SyslogKit {

    struct LogFilter {
        std::string search_text;
        int min_severity = -1;
        int limit = 50;
    };

    class LogStorage {
    public:
        LogStorage();
        ~LogStorage();

        bool open(const std::string& path);
        void close();
        bool write(const SyslogMessage& msg);
        std::vector<SyslogMessage> query(const LogFilter& filter);

        [[nodiscard]] std::string get_db_path() const { return db_path_; }
        [[nodiscard]] bool is_open() const { return db_ != nullptr; }

    private:
        void init_table();
        sqlite3* db_ = nullptr;
        std::string db_path_;
    };
}