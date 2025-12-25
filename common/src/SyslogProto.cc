#include "SyslogKit/SyslogProto"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace SyslogKit {

    static std::string get_time_rfc3164() {
        std::time_t t = std::time(nullptr);
        std::tm tm_buf{};
        #if defined(_WIN32)
                localtime_s(&tm_buf, &t);
        #else
                localtime_r(&t, &tm_buf);
        #endif
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%s %2d %02d:%02d:%02d",
                 months[tm_buf.tm_mon], tm_buf.tm_mday,
                 tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
        return {buffer};
    }

    std::string SyslogBuilder::build(const SyslogMessage& msg) {
        std::ostringstream oss;
        oss << "<" << ((static_cast<int>(msg.facility) * 8) + static_cast<int>(msg.severity)) << ">";

        oss << (msg.timestamp.empty() ? get_time_rfc3164() : msg.timestamp) << " ";

        oss << (msg.hostname.empty() ? "localhost" : msg.hostname) << " ";

        if (!msg.app_name.empty()) {
            oss << msg.app_name << ": ";
        }

        oss << msg.message;

        return oss.str();
    }

    SyslogMessage SyslogBuilder::parse(std::string_view raw_msg) {
        SyslogMessage msg;
        if (raw_msg.empty()) return msg;

        size_t pos = 0;
        if (raw_msg[0] == '<') {
            auto end_pri = raw_msg.find('>');
            if (end_pri != std::string_view::npos) {
                int pri = std::stoi(std::string(raw_msg.substr(1, end_pri - 1)));
                msg.facility = static_cast<Facility>(pri / 8);
                msg.severity = static_cast<Severity>(pri % 8);
                pos = end_pri + 1;
            }
        }
        size_t space_count = 0;
        size_t last_pos = pos;
        for (size_t i = pos; i < raw_msg.size() && space_count < 3; ++i) {
            if (raw_msg[i] == ' ') {
                space_count++;
                last_pos = i + 1;
            }
        }

        const auto next_space = raw_msg.find(' ', last_pos);
        if (next_space != std::string_view::npos) {
            last_pos = next_space + 1;
        }

        msg.message = std::string(raw_msg.substr(last_pos));

        return msg;
    }

} // namespace syslog