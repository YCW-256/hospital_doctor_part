#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "MyTcp/socketlink.h"
#include "pans/loginwidget.h"
#include "pans/syswidget.h"
#include "pans/appointwidget.h"
#include "pans/guardwidget.h"
#include "pans/orderwidget.h"
#include "pans/recordwidget.h"
#include "pans/workstatwidget.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    SocketLink * m_socket;

    explicit MainWindow(SocketLink * socket,QWidget *parent = nullptr);
    ~MainWindow() override;

    void init_connect();

    void init_task_connect();

    void init_stack_widget();

signals:
    void sendok(QByteArray,int size);
private:
    Ui::MainWindow *ui;


    LoginWidget *login_widget;


    SysWidget* sys_widget;

    AppointWidget* appoint_widget;

    GuardWidget* guard_widget;

    OrderWidget* order_widget;

    WorkStatWidget* workstat_widget;   // 工作统计页（普通医生端）

    RecordWidget* record_widget;       // 查看病例（病历）页（普通医生端）

};
#endif // MAINWINDOW_H
