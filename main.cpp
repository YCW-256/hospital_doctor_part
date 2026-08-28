#include "mainwindow.h"
#include "pans/loginwidget.h"
#include <QApplication>
#include "MyTcp/cdata.h"
#include "pans/childs/medicalcardwidget.h"
#include "pans/childs/selbtn.h"
#include "pans/appointwidget.h"
#include "pans/childs/customcard.h"
#include "pans/guardwidget.h"
#include "widget.h"
#include "pans/orderwidget.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CData::init();

    MainWindow w;

    //CustomCard ca;
    //ca.show();

    //GuardWidget g;
    //g.show();
    //Widget b;
    //b.show();
    // OrderWidget o;
    // o.show();

    return QApplication::exec();
}
