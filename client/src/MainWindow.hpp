#pragma once

#include <QMainWindow>
#include <QAbstractTableModel>
#include <vector>
#include "SyslogKit/SyslogServer"
#include "SyslogKit/LogStorage"

class QTableView;
class QLabel;
class QPushButton;
class QLineEdit;

class SyslogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SyslogModel(QObject* parent = nullptr);
    [[nodiscard]] int rowCount(const QModelIndex&) const override;
    [[nodiscard]] int columnCount(const QModelIndex&) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void add(const SyslogKit::SyslogMessage& msg);
    void set(const std::vector<SyslogKit::SyslogMessage>& msgs);
    void clear();
    [[nodiscard]] const std::vector<SyslogKit::SyslogMessage>& items() const { return data_; }

private:
    std::vector<SyslogKit::SyslogMessage> data_;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow() override;

    signals:
        void logReceived(QString fac, QString sev, QString host, QString app, QString msg, QString time);

private slots:
    void onToggleServer();
    void onLogReceived(const QString &fac, const QString &sev, const QString &host, const QString &app, const QString &msg, const QString &time) const;
    void onRefreshDb();
    void onExport();

private:
    void setupUi();
    void setupTheme();

    SyslogKit::Server server_;
    SyslogKit::LogStorage storage_;
    bool isRunning_ = false;

    SyslogModel* liveModel_{};
    SyslogModel* dbModel_{};
    QTableView* liveView_{};
    QTableView* dbView_{};
    QPushButton* btnStart_{};
    QLabel* statusLbl_{};
    QLineEdit* searchEdit_{};
};