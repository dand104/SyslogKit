#pragma once

#include <QAbstractTableModel>
#include <vector>
#include "SyslogKit/SyslogProto.hxx"

class SyslogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SyslogModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void add(const SyslogKit::SyslogMessage& msg);
    void set(const std::vector<SyslogKit::SyslogMessage>& msgs);
    void clear();
    [[nodiscard]] const std::vector<SyslogKit::SyslogMessage>& items() const { return data_; }
    [[nodiscard]] const SyslogKit::SyslogMessage* getItem(int row) const;

private:
    std::vector<SyslogKit::SyslogMessage> data_;
};