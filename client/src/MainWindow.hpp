#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QUdpSocket>
#include <QTcpSocket>

class SyslogClientWidget : public QWidget {
    Q_OBJECT
public:
    explicit SyslogClientWidget(QWidget *parent = nullptr);

private slots:
    void sendData() const;

private:
    QLineEdit *ipEdit, *portEdit, *msgEdit, *tagEdit;
    QComboBox *severityCombo, *facilityCombo, *protoCombo;
    QLabel *statusLabel;
    QUdpSocket *udpSocket;
    QTcpSocket *tcpSocket;

};