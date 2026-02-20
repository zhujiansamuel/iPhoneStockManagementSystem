#include <QApplication>
#include "mainwindow.h"
#include <QtCore/qresource.h>


#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/app.png"));
    MainWindow w;
    w.show();
    return a.exec();
}
