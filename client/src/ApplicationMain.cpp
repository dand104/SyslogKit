#include <QApplication>
#include "MainWindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    SyslogClientWidget window;
    window.setWindowTitle("Syslog Kit Client");
    window.show();
    return QApplication::exec();
}