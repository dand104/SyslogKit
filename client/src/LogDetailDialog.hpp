#pragma once

#include <QDialog>
#include "SyslogKit/SyslogProto.hxx"

class LogDetailDialog : public QDialog {
    Q_OBJECT
public:
    explicit LogDetailDialog(const SyslogKit::SyslogMessage& msg, QWidget* parent = nullptr);
};