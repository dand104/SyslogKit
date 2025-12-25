#include "MainWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableView>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QDateTime>


SyslogModel::SyslogModel(QObject* p) : QAbstractTableModel(p) {}

int SyslogModel::rowCount(const QModelIndex&) const { return static_cast<int>(data_.size()); }
int SyslogModel::columnCount(const QModelIndex&) const { return 6; }

QVariant SyslogModel::data(const QModelIndex& idx, const int role) const {
    if (idx.row() >= data_.size()) return {};
    const auto&[facility, severity, timestamp, hostname, app_name, message] = data_[idx.row()];

    if (role == Qt::DisplayRole) {
        switch(idx.column()) {
            case 0: return QString::fromStdString(timestamp);
            case 1: return QString::number(static_cast<int>(facility));
            case 2: {
                static const char* s[] = {"Emerg","Alert","Crit","Error","Warn","Notice","Info","Debug"};
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
        const char* h[] = {"Time", "Fac", "Sev", "Host", "App", "Message"};
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
    beginInsertRows({}, data_.size(), data_.size());
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

// --- Main Window ---

MainWindow::MainWindow() {
    setupTheme();
    setupUi();

    const QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbDir);
    const std::string dbPath = (dbDir + "/SyslogKit_Local.db").toStdString();

    if (!storage_.open(dbPath)) {
        QMessageBox::critical(this, "Error", "Failed to open database!");
    }

    connect(this, &MainWindow::logReceived, this, &MainWindow::onLogReceived);

    server_.set_callback([this](SyslogKit::SyslogMessage msg) {
        storage_.write(msg);

        emit logReceived(
            QString::number(static_cast<int>(msg.facility)),
            QString::number(static_cast<int>(msg.severity)),
            QString::fromStdString(msg.hostname),
            QString::fromStdString(msg.app_name),
            QString::fromStdString(msg.message),
            QString::fromStdString(msg.timestamp)
        );
    });
}

MainWindow::~MainWindow() {
    server_.stop();
}

void MainWindow::setupTheme() {
    qApp->setStyle("Fusion");
    QPalette p = qApp->palette();
    p.setColor(QPalette::Window, QColor(53, 53, 53));
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Base, QColor(25, 25, 25));
    p.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    p.setColor(QPalette::ToolTipBase, Qt::white);
    p.setColor(QPalette::ToolTipText, Qt::white);
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, QColor(53, 53, 53));
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, QColor(42, 130, 218));
    p.setColor(QPalette::Highlight, QColor(66, 165, 245));
    p.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(p);
}

void MainWindow::setupUi() {
    resize(1100, 700);
    setWindowTitle("Syslog Kit - C++20 Qt6");

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLay = new QVBoxLayout(central);

    auto* tabs = new QTabWidget();
    mainLay->addWidget(tabs);

    auto* liveTab = new QWidget();
    auto* liveLay = new QVBoxLayout(liveTab);

    auto* topBar = new QHBoxLayout();
    btnStart_ = new QPushButton("Start Server (5140)");
    btnStart_->setCheckable(true);
    btnStart_->setStyleSheet("QPushButton:checked { background-color: #EF5350; }");
    connect(btnStart_, &QPushButton::clicked, this, &MainWindow::onToggleServer);

    statusLbl_ = new QLabel("Stopped");
    statusLbl_->setStyleSheet("color: gray; font-weight: bold;");

    topBar->addWidget(btnStart_);
    topBar->addWidget(statusLbl_);
    topBar->addStretch();

    auto* btnClear = new QPushButton("Clear");
    connect(btnClear, &QPushButton::clicked, [this](){ liveModel_->clear(); });
    topBar->addWidget(btnClear);

    liveView_ = new QTableView();
    liveModel_ = new SyslogModel(this);
    liveView_->setModel(liveModel_);
    liveView_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    liveView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    liveView_->verticalHeader()->setVisible(false);
    liveView_->setShowGrid(false);

    liveLay->addLayout(topBar);
    liveLay->addWidget(liveView_);
    tabs->addTab(liveTab, "Live Monitor");

    auto* dbTab = new QWidget();
    auto* dbLay = new QVBoxLayout(dbTab);

    auto* filterBar = new QHBoxLayout();
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Filter by message content...");
    auto* btnSearch = new QPushButton("Search");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::onRefreshDb);
    auto* btnExport = new QPushButton("Export");
    connect(btnExport, &QPushButton::clicked, this, &MainWindow::onExport);

    filterBar->addWidget(searchEdit_);
    filterBar->addWidget(btnSearch);
    filterBar->addWidget(btnExport);

    dbView_ = new QTableView();
    dbModel_ = new SyslogModel(this);
    dbView_->setModel(dbModel_);
    dbView_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    dbView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView_->verticalHeader()->setVisible(false);

    dbLay->addLayout(filterBar);
    dbLay->addWidget(dbView_);
    tabs->addTab(dbTab, "History (SQLite)");
}

void MainWindow::onToggleServer() {
    if (isRunning_) {
        server_.stop();
        statusLbl_->setText("Stopped");
        statusLbl_->setStyleSheet("color: gray; font-weight: bold;");
        btnStart_->setText("Start Server (5140)");
        isRunning_ = false;
    } else {
        try {
            // Используем порт 5140, чтобы не требовать права рута/админа
            server_.start(5140, true, true);
            statusLbl_->setText("Running UDP/TCP :5140");
            statusLbl_->setStyleSheet("color: #66BB6A; font-weight: bold;");
            btnStart_->setText("Stop Server");
            isRunning_ = true;
        } catch (...) {
            QMessageBox::warning(this, "Error", "Could not bind port.");
            btnStart_->setChecked(false);
        }
    }
}

void MainWindow::onLogReceived(const QString &fac, const QString &sev, const QString &host, const QString &app, const QString &msg, const QString &time) const {
    SyslogKit::SyslogMessage m;
    m.facility = static_cast<SyslogKit::Facility>(fac.toInt());
    m.severity = static_cast<SyslogKit::Severity>(sev.toInt());
    m.hostname = host.toStdString();
    m.app_name = app.toStdString();
    m.message = msg.toStdString();
    m.timestamp = time.toStdString();

    liveModel_->add(m);
    liveView_->scrollToBottom();
}

void MainWindow::onRefreshDb() {
    SyslogKit::LogFilter filter;
    filter.search_text = searchEdit_->text().toStdString();
    auto logs = storage_.query(filter);
    dbModel_->set(logs);
}

void MainWindow::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export", "", "Log File (*.log)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        for (const auto& m : dbModel_->items()) {
            out << QString::fromStdString(m.timestamp) << "\t"
                << QString::fromStdString(m.hostname) << "\t"
                << QString::fromStdString(m.message) << "\n";
        }
    }
}