#include "mainwindow.h"
#include "pans/loginwidget.h"
#include <QApplication>
#include "MyTcp/cdata.h"
#include "pans/childs/medicalcardwidget.h"
#include "pans/childs/selbtn.h"
#include "pans/appointwidget.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CData::init();

    MainWindow w;




    return QApplication::exec();
}
