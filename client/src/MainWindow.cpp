#include "MainWindow.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QNetworkProxy>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QHostAddress>
#include "SyslogKit/SyslogProto"

SyslogClientWidget::SyslogClientWidget(QWidget *parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    protoCombo = new QComboBox(this);
    protoCombo->addItems({"UDP", "TCP"});
    ipEdit = new QLineEdit("127.0.0.1", this);
    portEdit = new QLineEdit("5140", this);

    facilityCombo = new QComboBox(this);
    facilityCombo->addItem("User", 1);
    facilityCombo->addItem("Local0", 16);
    facilityCombo->addItem("Auth", 4);

    severityCombo = new QComboBox(this);
    const char* sevLevels[] = {"Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Info", "Debug"};
    for(int i=0; i<8; ++i) severityCombo->addItem(sevLevels[i], i);
    severityCombo->setCurrentIndex(6);

    tagEdit = new QLineEdit("QtApp", this);
    msgEdit = new QLineEdit("Test message", this);
    statusLabel = new QLabel("Ready", this);
    auto* sendBtn = new QPushButton("Send Message", this);

    layout->addWidget(new QLabel("Protocol & Address:"));
    layout->addWidget(protoCombo);
    layout->addWidget(ipEdit);
    layout->addWidget(portEdit);
    layout->addWidget(new QLabel("Facility & Severity:"));
    layout->addWidget(facilityCombo);
    layout->addWidget(severityCombo);
    layout->addWidget(new QLabel("Tag & Message:"));
    layout->addWidget(tagEdit);
    layout->addWidget(msgEdit);
    layout->addWidget(sendBtn);
    layout->addWidget(statusLabel);

    udpSocket = new QUdpSocket(this);
    udpSocket->setProxy(QNetworkProxy::NoProxy);

    tcpSocket = new QTcpSocket(this);
    tcpSocket->setProxy(QNetworkProxy::NoProxy);

    connect(sendBtn, &QPushButton::clicked, this, &SyslogClientWidget::sendData);
}

void SyslogClientWidget::sendData() const {
    SyslogKit::SyslogMessage msg;
    msg.facility = static_cast<SyslogKit::Facility>(facilityCombo->currentData().toInt());
    msg.severity = static_cast<SyslogKit::Severity>(severityCombo->currentData().toInt());
    msg.app_name = tagEdit->text().toStdString();
    msg.message = msgEdit->text().toStdString();

    const std::string payload = SyslogKit::SyslogBuilder::build(msg);
    const QHostAddress addr(ipEdit->text());
    const quint16 port = portEdit->text().toUInt();

    if (protoCombo->currentText() == "UDP") {
        udpSocket->writeDatagram(QByteArray::fromStdString(payload), addr, port);
        statusLabel->setText("UDP Sent");
    } else {
        if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
            tcpSocket->connectToHost(addr, port);
            if (!tcpSocket->waitForConnected(1000)) {
                statusLabel->setText("TCP Connection Failed");
                return;
            }
        }
        tcpSocket->write(QByteArray::fromStdString(payload + "\n"));
        statusLabel->setText("TCP Sent");
    }
}
