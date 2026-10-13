#include "SyslogModel.hpp"
#include <QColor>

SyslogModel::SyslogModel(QObject* p) : QAbstractTableModel(p) {}

int SyslogModel::rowCount(const QModelIndex&) const { return static_cast<int>(data_.size()); }
int SyslogModel::columnCount(const QModelIndex&) const { return 6; }

const SyslogKit::SyslogMessage* SyslogModel::getItem(int row) const {
    if (row >= 0 && row < static_cast<int>(data_.size())) {
        return &data_[row];
    }
    return nullptr;
}

QVariant SyslogModel::data(const QModelIndex& idx, const int role) const {
    if (idx.row() >= static_cast<int>(data_.size())) return {};
    const auto& [facility, severity, timestamp, hostname, app_name, message] = data_[idx.row()];

    if (role == Qt::DisplayRole) {
        switch (idx.column()) {
            case 0: return QString::fromStdString(timestamp);
            case 1: return QString::number(static_cast<int>(facility));
            case 2: {
                static const char* s[] = {"Emerg", "Alert", "Crit", "Error", "Warn", "Notice", "Info", "Debug"};
                const int sev = static_cast<int>(severity);
                return (sev >= 0 && sev <= 7) ? s[sev] : "Unk";
            }
            case 3: return QString::fromStdString(hostname);
            case 4: return QString::fromStdString(app_name);
            case 5: return QString::fromStdString(message);
        }
    } else if (role == Qt::ForegroundRole) {
        if (static_cast<int>(severity) <= 3) return QColor(0xFF5252);
        if (static_cast<int>(severity) == 4) return QColor(0xFFB74D);
    }
    return {};
}

QVariant SyslogModel::headerData(const int sec, const Qt::Orientation ori, const int role) const {
    if (ori == Qt::Horizontal && role == Qt::DisplayRole) {
        static const char* h[] = {"Time", "Fac", "Sev", "Host", "App", "Message"};
        return h[sec];
    }
    return {};
}

void SyslogModel::add(const SyslogKit::SyslogMessage& msg) {
    if (data_.size() > 5000) {
        beginRemoveRows({}, 0, 0);
        data_.erase(data_.begin());
        endRemoveRows();
    }
    beginInsertRows({}, static_cast<int>(data_.size()), static_cast<int>(data_.size()));
    data_.push_back(msg);
    endInsertRows();
}

void SyslogModel::set(const std::vector<SyslogKit::SyslogMessage>& msgs) {
    beginResetModel();
    data_ = msgs;
    endResetModel();
}

void SyslogModel::clear() {
    beginResetModel();
    data_.clear();
    endResetModel();
}