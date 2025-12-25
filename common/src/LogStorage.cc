#include "SyslogKit/LogStorage"
#include <sqlite3.h>
#include <iostream>

namespace SyslogKit {

    LogStorage::LogStorage() = default;
    LogStorage::~LogStorage() { close(); }

    void LogStorage::close() {
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }

    bool LogStorage::open(const std::string& path) {
        if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) return false;
        init_table();
        return true;
    }

    void LogStorage::init_table() {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS logs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                fac INTEGER, sev INTEGER,
                ts TEXT, host TEXT, app TEXT, msg TEXT
            );
            CREATE INDEX IF NOT EXISTS idx_ts ON logs(ts);
        )";
        sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    }

    bool LogStorage::write(const SyslogMessage& msg) {
        if (!db_) return false;
        const char* sql = "INSERT INTO logs (fac, sev, ts, host, app, msg) VALUES (?,?,?,?,?,?)";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, static_cast<int>(msg.facility));
        sqlite3_bind_int(stmt, 2, static_cast<int>(msg.severity));
        sqlite3_bind_text(stmt, 3, msg.timestamp.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, msg.hostname.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, msg.app_name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, msg.message.c_str(), -1, SQLITE_STATIC);

        const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
    }

    std::vector<SyslogMessage> LogStorage::query(const LogFilter& filter) {
        std::vector<SyslogMessage> res;
        if (!db_) return res;

        std::string sql = "SELECT fac, sev, ts, host, app, msg FROM logs WHERE 1=1";
        if (!filter.search_text.empty()) sql += " AND msg LIKE ?";
        sql += " ORDER BY id DESC LIMIT ?";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return res;

        int idx = 1;
        if (!filter.search_text.empty()) {
            const std::string s = "%" + filter.search_text + "%";
            sqlite3_bind_text(stmt, idx++, s.c_str(), -1, SQLITE_TRANSIENT);
        }
        sqlite3_bind_int(stmt, idx++, filter.limit);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SyslogMessage m;
            m.facility = static_cast<Facility>(sqlite3_column_int(stmt, 0));
            m.severity = static_cast<Severity>(sqlite3_column_int(stmt, 1));
            m.timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
            m.hostname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
            m.app_name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
            m.message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5));
            res.push_back(m);
        }
        sqlite3_finalize(stmt);
        return res;
    }
}