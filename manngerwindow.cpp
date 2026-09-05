#include "manngerwindow.h"
#include "ui_manngerwindow.h"
#include "QByteArray"
#include <QThread>
#include <QDebug>
#include "Tool/readutil.h"
#include "MyTcp/cdata.h"

ManngerWindow::ManngerWindow(SocketLink * socket,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ManngerWindow)
{
    this->m_socket=socket;
    this->setMinimumSize(CData::current_width,CData::current_height);

    ui->setupUi(this);

    init_stack_widget();

    ui->treeWidget->setObjectName("treeSideMenu");
    ReadUtil::setWidgetQss(ui->treeWidget, ":/qss/tree.qss");

    QThread *thread=new QThread;
    m_socket->moveToThread(thread);
    thread->start();
    qDebug()<<"【主循环pid】"<<QThread::currentThreadId();
    init_task_connect();

    init_connect();
}

ManngerWindow::~ManngerWindow()
{
    delete ui;
}

void ManngerWindow::init_connect()
{
    ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(0));

    // 首页图标按钮：值班信息/查看预约 都导向唯一的业务页(排班表)
    connect(sys_widget,&SysWidget::to_guard_page,this,[this](){
        ui->stackedWidget->setCurrentWidget(doctor_order);
        CData::current_widget=doctor_order;
        ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(5));
    });
    connect(sys_widget,&SysWidget::to_app_page,this,[this](){
        ui->stackedWidget->setCurrentWidget(doctor_order);
        CData::current_widget=doctor_order;
        ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(5));
    });

    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int column){
        Q_UNUSED(column);
        int index = ui->treeWidget->indexOfTopLevelItem(item);

        switch(index){
        case 0:{ // 开始界面
            ui->stackedWidget->setCurrentWidget(sys_widget);
            CData::current_widget=sys_widget;
            break;
        }
        case 3: // 值班信息
        case 5: // 值班管理(排班入口)
        case 6: // 值班信息(重复项)
            ui->stackedWidget->setCurrentWidget(doctor_order);
            CData::current_widget=doctor_order;
            break;
        default:
            break; // 查看病例/工作统计/查看预约 暂无对应页
        }
    });
}

void ManngerWindow::init_task_connect()
{
    // 排班流：DoctorOrder 选科室 -> 查医生列表，socket 回包后刷新纵轴
    connect(this->doctor_order,&DoctorOrder::get_doctor_info,this->m_socket,&SocketLink::send_data);
    connect(this->m_socket,&SocketLink::get_doctor_info_success,this->doctor_order,&DoctorOrder::flush_doctor);

    // 排班流：DoctorOrder 查本周排班(GET_GUARD)，回包后把排班显示到卡片
    connect(this->doctor_order,&DoctorOrder::get_guard_info,this->m_socket,&SocketLink::send_data);
    connect(this->m_socket,&SocketLink::get_guard_info_success,this->doctor_order,&DoctorOrder::flush_table);

    // 排班流：保存修改(REPIX_GUARD)
    connect(this->doctor_order,&DoctorOrder::save_guard_info,this->m_socket,&SocketLink::send_data);

    connect(this->m_socket,&SocketLink::login_success,this,[this](){
        this->show();
    });
}

void ManngerWindow::init_stack_widget()
{
    sys_widget=new SysWidget(this);

    doctor_order=new DoctorOrder(this);

    ui->stackedWidget->addWidget(sys_widget);

    ui->stackedWidget->addWidget(doctor_order);
    //设置默认页（系统页）
    ui->stackedWidget->setCurrentWidget(sys_widget);
    CData::current_widget=sys_widget;
}
