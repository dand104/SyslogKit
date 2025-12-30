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
#include <QTextEdit>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QCheckBox>

class LogDetailDialog : public QDialog {
public:
    explicit LogDetailDialog(const SyslogKit::SyslogMessage& msg, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle("Log Details");
        resize(600, 400);
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
};

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

MainWindow::MainWindow() : settings_() {
    setupUi();
    const QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbDir);
    std::string dbPath = (dbDir + "/SyslogKit_Local.db").toStdString();

    if (!storage_.open(dbPath)) {
        QMessageBox::critical(this, "Error", "Failed to open default database!");
    } else {
        currentDbLbl_->setText("DB: " + QString::fromStdString(dbPath));
    }

    connect(this, &MainWindow::logReceived, this, &MainWindow::onLogReceived);

    server_.set_callback([this](SyslogKit::SyslogMessage const& msg) {
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
    loadSettings();
}

MainWindow::~MainWindow() {
    server_.stop();
}

void MainWindow::setupUi() {
    resize(1100, 700);
    setWindowTitle("SyslogKit");

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLay = new QVBoxLayout(central);

    tabs_ = new QTabWidget();
    connect(tabs_, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    mainLay->addWidget(tabs_);

    auto* liveTab = new QWidget();
    auto* liveLay = new QVBoxLayout(liveTab);

    auto* topBar = new QHBoxLayout();
    btnStart_ = new QPushButton("Start Server");
    btnStart_->setCheckable(true);
    btnStart_->setStyleSheet("QPushButton:checked { background-color: #EF5350; color: white; }");
    connect(btnStart_, &QPushButton::clicked, this, &MainWindow::onToggleServer);

    statusLbl_ = new QLabel("Stopped");
    statusLbl_->setStyleSheet("color: gray; font-weight: bold;");

    auto* btnClear = new QPushButton("Clear View");
    connect(btnClear, &QPushButton::clicked, [this](){ liveModel_->clear(); });

    topBar->addWidget(btnStart_);
    topBar->addWidget(statusLbl_);
    topBar->addStretch();
    topBar->addWidget(btnClear);

    liveView_ = new QTableView();
    liveModel_ = new SyslogModel(this);
    liveView_->setModel(liveModel_);
    liveView_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    liveView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    liveView_->setAlternatingRowColors(true);
    liveView_->verticalHeader()->setVisible(false);
    liveView_->setShowGrid(false);
    connect(liveView_, &QTableView::doubleClicked, this, &MainWindow::onTableDoubleClicked);

    liveLay->addLayout(topBar);
    liveLay->addWidget(liveView_);
    tabs_->addTab(liveTab, "Live Monitor");

    auto* dbTab = new QWidget();
    auto* dbLay = new QVBoxLayout(dbTab);

    auto* dbToolsBar = new QHBoxLayout();

    auto* btnSwitchDb = new QPushButton("Open/Switch DB");
    connect(btnSwitchDb, &QPushButton::clicked, this, &MainWindow::onSwitchDb);
    auto* btnExportDb = new QPushButton("Export DB File");
    connect(btnExportDb, &QPushButton::clicked, this, &MainWindow::onExportDb);

    currentDbLbl_ = new QLabel("DB: <none>");
    currentDbLbl_->setStyleSheet("color: #555; font-size: 10px;");

    dbToolsBar->addWidget(new QLabel("Database:"));
    dbToolsBar->addWidget(btnSwitchDb);
    dbToolsBar->addWidget(btnExportDb);
    dbToolsBar->addStretch();
    dbToolsBar->addWidget(currentDbLbl_);

    auto* filterBar = new QHBoxLayout();
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search message...");
    connect(searchEdit_, &QLineEdit::returnPressed, this, &MainWindow::onRefreshDb);

    limitCombo_ = new QComboBox();
    limitCombo_->addItem("Show: 20", 20);
    limitCombo_->addItem("Show: 50", 50);
    limitCombo_->addItem("Show: 100", 100);
    limitCombo_->addItem("Show: 500", 500);
    limitCombo_->addItem("Show: All", 0);
    limitCombo_->setCurrentIndex(1); // Default 50
    connect(limitCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::onRefreshDb);

    auto* btnSearch = new QPushButton("Refresh / Search");
    connect(btnSearch, &QPushButton::clicked, this, &MainWindow::onRefreshDb);

    auto* btnExportLogs = new QPushButton("Export Logs (.log)");
    connect(btnExportLogs, &QPushButton::clicked, this, &MainWindow::onExportLogs);

    filterBar->addWidget(searchEdit_);
    filterBar->addWidget(limitCombo_);
    filterBar->addWidget(btnSearch);
    filterBar->addWidget(btnExportLogs);

    dbView_ = new QTableView();
    dbModel_ = new SyslogModel(this);
    dbView_->setModel(dbModel_);
    dbView_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    dbView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    dbView_->setAlternatingRowColors(true);
    dbView_->verticalHeader()->setVisible(false);
    connect(dbView_, &QTableView::doubleClicked, this, &MainWindow::onTableDoubleClicked);

    dbLay->addLayout(dbToolsBar);
    dbLay->addLayout(filterBar);
    dbLay->addWidget(dbView_);
    tabs_->addTab(dbTab, "Database");

    auto* setTab = new QWidget();
    auto* setLay = new QVBoxLayout(setTab);

    auto* grpServer = new QGroupBox("Server Settings");
    auto* srvLay = new QFormLayout(grpServer);
    portSpin_ = new QSpinBox();
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(5140);
    srvLay->addRow("UDP/TCP Port:", portSpin_);

    auto* protoLay = new QHBoxLayout();
    chkUdp_ = new QCheckBox("Enable UDP");
    chkUdp_->setChecked(true);
    chkTcp_ = new QCheckBox("Enable TCP");
    chkTcp_->setChecked(true);
    protoLay->addWidget(chkUdp_);
    protoLay->addWidget(chkTcp_);
    protoLay->addStretch();

    srvLay->addRow("Protocols:", protoLay);

    auto* grpGui = new QGroupBox("Interface Settings");
    auto* guiLay = new QFormLayout(grpGui);
    defaultLimitCombo_ = new QComboBox();
    defaultLimitCombo_->addItem("20", 20);
    defaultLimitCombo_->addItem("50", 50);
    defaultLimitCombo_->addItem("100", 100);
    guiLay->addRow("Default DB View Limit:", defaultLimitCombo_);

    auto* btnLay = new QHBoxLayout();
    auto* btnSaveSet = new QPushButton("Save Settings");
    connect(btnSaveSet, &QPushButton::clicked, this, &MainWindow::onSaveSettings);

    auto* btnRestore = new QPushButton("Restore Defaults");
    connect(btnRestore, &QPushButton::clicked, this, &MainWindow::onRestoreDefaults);

    btnLay->addWidget(btnSaveSet);
    btnLay->addWidget(btnRestore);
    btnLay->addStretch();

    setLay->addWidget(grpServer);
    setLay->addWidget(grpGui);
    setLay->addLayout(btnLay);
    setLay->addStretch();

    tabs_->addTab(setTab, "Settings");
}
void MainWindow::loadSettings() const
{
    const int port = settings_.value("server/port", 5140).toInt();
    portSpin_->setValue(port);

    const bool udp = settings_.value("server/udp_enabled", true).toBool();
    const bool tcp = settings_.value("server/tcp_enabled", true).toBool();
    chkUdp_->setChecked(udp);
    chkTcp_->setChecked(tcp);

    const int defLimit = settings_.value("gui/db_limit", 50).toInt();
    int idx = defaultLimitCombo_->findData(defLimit);
    if (idx >= 0) defaultLimitCombo_->setCurrentIndex(idx);

    idx = limitCombo_->findData(defLimit);
    if (idx >= 0) limitCombo_->setCurrentIndex(idx);
}

void MainWindow::onSaveSettings() {
    settings_.setValue("server/port", portSpin_->value());
    settings_.setValue("server/udp_enabled", chkUdp_->isChecked());
    settings_.setValue("server/tcp_enabled", chkTcp_->isChecked());
    settings_.setValue("gui/db_limit", defaultLimitCombo_->currentData().toInt());
    settings_.sync();

    int idx = limitCombo_->findData(defaultLimitCombo_->currentData().toInt());
    if (idx >= 0) limitCombo_->setCurrentIndex(idx);

    QMessageBox::information(this, "Settings", "Settings saved. Restart server to apply port changes.");
}

void MainWindow::onTabChanged(const int index) {
    if (index == 1) {
        onRefreshDb();
    }
}
void MainWindow::onToggleServer() {
    if (isRunning_) {
        server_.stop();
        statusLbl_->setText("Stopped");
        statusLbl_->setStyleSheet("color: gray; font-weight: bold;");
        const int port = portSpin_->value();
        btnStart_->setText(QString("Start Server (%1)").arg(port));
        isRunning_ = false;
    } else {
        const int port = portSpin_->value();
        const bool useUdp = chkUdp_->isChecked();
        const bool useTcp = chkTcp_->isChecked();

        if (!useUdp && !useTcp) {
            QMessageBox::warning(this, "Error", "Please enable at least one protocol (UDP or TCP) in settings.");
            btnStart_->setChecked(false);
            return;
        }

        try {
            server_.start(static_cast<uint16_t>(port), useUdp, useTcp);

            QStringList protos;
            if (useUdp) protos << "UDP";
            if (useTcp) protos << "TCP";

            statusLbl_->setText(QString("Running %1 :%2").arg(protos.join("/")).arg(port));
            statusLbl_->setStyleSheet("color: #66BB6A; font-weight: bold;");
            btnStart_->setText("Stop Server");
            isRunning_ = true;
        } catch (...) {
            QMessageBox::warning(this, "Error", "Could not bind port " + QString::number(port));
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
    const auto logs = storage_.query(filter);
    dbModel_->set(logs);
}

void MainWindow::onExportLogs() {
    const QString path = QFileDialog::getSaveFileName(this, "Export Logs", "", "Log File (*.log)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
        for (const auto& m : dbModel_->items()) {
            out << QString::fromStdString(m.timestamp) << "\t"
                << QString::fromStdString(m.hostname) << "\t"
                << QString::fromStdString(m.message) << "\n";
        }
        QMessageBox::information(this, "Success", "Logs exported successfully.");
    } else {
        QMessageBox::warning(this, "Error", "Could not write to file.");
    }
}

void MainWindow::onSwitchDb() {
    const QString path = QFileDialog::getOpenFileName(this, "Open Database", "", "SQLite DB (*.db *.sqlite);;All Files (*)");
    if (path.isEmpty()) return;

    if (storage_.open(path.toStdString())) {
        currentDbLbl_->setText("DB: " + path);
        onRefreshDb();
        QMessageBox::information(this, "Database", "Switched to new database. New logs will be written here.");
    } else {
        QMessageBox::critical(this, "Error", "Failed to open database file.");
    }
}

void MainWindow::onExportDb() {
    if (!storage_.is_open()) {
        QMessageBox::warning(this, "Warning", "No database is currently open.");
        return;
    }

    const QString srcPath = QString::fromStdString(storage_.get_db_path());
    const QString destPath = QFileDialog::getSaveFileName(this, "Export Database", "backup.db", "SQLite DB (*.db)");

    if (destPath.isEmpty()) return;

    if (QFile src(srcPath); src.copy(destPath)) {
        QMessageBox::information(this, "Success", "Database exported successfully.");
    } else {
        QMessageBox::warning(this, "Error", "Failed to copy database file. Check permissions.");
    }
}

void MainWindow::onTableDoubleClicked(const QModelIndex &index) {const SyslogModel* model = nullptr;
    if (sender() == liveView_) model = liveModel_;
    else if (sender() == dbView_) model = dbModel_;

    if (model) {
        if (const auto* item = model->getItem(index.row())) {
            showDetailDialog(*item);
        }
    }
}

void MainWindow::showDetailDialog(const SyslogKit::SyslogMessage& msg) {
    LogDetailDialog dlg(msg, this);
    dlg.exec();
}

void MainWindow::onRestoreDefaults() {
    if (QMessageBox::question(this, "Confirm", "Reset all settings to default values?") == QMessageBox::Yes) {
        portSpin_->setValue(5140);
        chkUdp_->setChecked(true);
        chkTcp_->setChecked(true);
        defaultLimitCombo_->setCurrentIndex(1);

        onSaveSettings();
    }
}