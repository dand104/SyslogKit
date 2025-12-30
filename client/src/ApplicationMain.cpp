#include <QApplication>
#include "MainWindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("SyslogKitProject");
    QCoreApplication::setApplicationName("SyslogKit");
    MainWindow window;
    window.setWindowTitle("Syslog Kit Client");
    window.show();
    return QApplication::exec();
}