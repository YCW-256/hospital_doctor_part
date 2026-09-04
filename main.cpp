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
#include "MyTcp/socketlink.h"
#include "manngerwindow.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CData::init();
    SocketLink* m_socket=new SocketLink;
    m_socket->connectHost();
    LoginWidget* login_widget = new LoginWidget(m_socket);
    // 绑定登录成功信号
    QObject::connect(login_widget, &LoginWidget::loginSuccess,
                     [login_widget, m_socket]()
                     {
                        // 登录成功，销毁登录弹窗
                        login_widget->deleteLater();
                        if(CData::m_role==0){
                         MainWindow*w = new MainWindow(m_socket,nullptr);
                         w->show();
                         qDebug()<<"用户登录成功，创建主窗口";
                        }
                        else if(CData::m_role==1){
                            ManngerWindow*w = new ManngerWindow(m_socket,nullptr);
                            w->show();
                            qDebug()<<"管理员登录成功，创建主窗口";
                        }

                     });



    login_widget->show();

    //MainWindow w(m_socket);




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
