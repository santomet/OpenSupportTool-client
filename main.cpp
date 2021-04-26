#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    QCoreApplication::setOrganizationName("Goal Sport Labs");
    QCoreApplication::setOrganizationDomain("goalsport.software");
    QCoreApplication::setApplicationName("Open Support Tool Client");

    MainWindow w;
    w.show();
    return a.exec();
}
