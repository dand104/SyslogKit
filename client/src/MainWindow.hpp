#pragma once

#include <QMainWindow>
#include <QSettings>
#include <vector>
#include "SyslogModel.hpp"
#include "SyslogKit/SyslogServer.hxx"
#include "SyslogKit/LogStorage.hxx"

class QTableView;
class QLabel;
class QPushButton;
class QLineEdit;
class QTabWidget;
class QComboBox;
class QSpinBox;
class QCheckBox;

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
    void onExportLogs();
    void onExportDb();
    void onSwitchDb();
    void onClearDb();
    void onTabChanged(int index);
    void onTableDoubleClicked(const QModelIndex &index);
    void onSaveSettings();
    void onRestoreDefaults();

private:
    void setupUi();
    void loadSettings() const;
    void showDetailDialog(const SyslogKit::SyslogMessage& msg);

    SyslogKit::Server server_;
    SyslogKit::LogStorage storage_;
    QSettings settings_;
    bool isRunning_ = false;

    QTabWidget* tabs_{};
    SyslogModel* liveModel_{};
    SyslogModel* dbModel_{};
    QTableView* liveView_{};
    QTableView* dbView_{};

    QPushButton* btnStart_{};
    QLabel* statusLbl_{};

    QLineEdit* searchEdit_{};
    QComboBox* limitCombo_{};
    QLabel* currentDbLbl_{};

    QSpinBox* portSpin_{};
    QCheckBox* chkUdp_{};
    QCheckBox* chkTcp_{};
    QComboBox* defaultLimitCombo_{};
};