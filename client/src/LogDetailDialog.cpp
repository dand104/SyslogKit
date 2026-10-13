#include "LogDetailDialog.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>

LogDetailDialog::LogDetailDialog(const SyslogKit::SyslogMessage& msg, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Log Details");
    resize(600, 400);
    setAttribute(Qt::WA_DeleteOnClose);

    auto* lay = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    auto addRow = [&](const QString& label, const QString& val) {
        auto* le = new QLineEdit(val);
        le->setReadOnly(true);
        form->addRow(label + ":", le);
    };

    addRow("Timestamp", QString::fromStdString(msg.timestamp));
    addRow("Hostname", QString::fromStdString(msg.hostname));
    addRow("App Name", QString::fromStdString(msg.app_name));
    addRow("Facility", QString::number(static_cast<int>(msg.facility)));
    addRow("Severity", QString::number(static_cast<int>(msg.severity)));

    lay->addLayout(form);

    lay->addWidget(new QLabel("Message:"));
    auto* textEdit = new QTextEdit();
    textEdit->setPlainText(QString::fromStdString(msg.message));
    textEdit->setReadOnly(true);
    lay->addWidget(textEdit);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    lay->addWidget(closeBtn, 0, Qt::AlignRight);
}