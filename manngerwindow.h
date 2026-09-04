#ifndef MANNGERWINDOW_H
#define MANNGERWINDOW_H

#include <QMainWindow>
#include "MyTcp/socketlink.h"
#include "pans/loginwidget.h"
#include "pans/syswidget.h"
#include "pans/appointwidget.h"
#include "pans/guardwidget.h"
#include "pans/orderwidget.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class ManngerWindow;
}
QT_END_NAMESPACE

class ManngerWindow : public QMainWindow
{
    Q_OBJECT

public:
    SocketLink * m_socket;

    explicit ManngerWindow(SocketLink * socket,QWidget *parent = nullptr);
    ~ManngerWindow() override;

    void init_connect();

    void init_task_connect();

    void init_stack_widget();

signals:
    void sendok(QByteArray,int size);
private:
    Ui::ManngerWindow *ui;


    LoginWidget *login_widget;


    SysWidget* sys_widget;

    AppointWidget* appoint_widget;

    GuardWidget* guard_widget;

    OrderWidget* order_widget;

};
#endif // MANNGERWINDOW_H
