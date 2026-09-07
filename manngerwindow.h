#ifndef MANNGERWINDOW_H
#define MANNGERWINDOW_H

#include <QMainWindow>
#include "MyTcp/socketlink.h"
#include "pans/syswidget.h"
#include "pans/doctororder.h"
#include "pans/mannger/managerstatwidget.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class ManngerWindow;
}
QT_END_NAMESPACE

// 管理员窗口：保留左侧导航 + 首页(SysWidget)，业务页只放一个排班页 DoctorOrder。
// 已移除从用户窗口拷贝过来的 appoint/guard/order 三页。
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

private:
    Ui::ManngerWindow *ui;

    SysWidget* sys_widget;

    DoctorOrder* doctor_order;

    ManagerStatWidget* workstat_widget;   // 工作统计页（管理员端）
};
#endif // MANNGERWINDOW_H
