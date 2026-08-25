#include "mainwindow.h"
#include "pans/loginwidget.h"
#include <QApplication>
#include "MyTcp/cdata.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CData::init();

    MainWindow w;
    w.show();



    return QApplication::exec();
}
