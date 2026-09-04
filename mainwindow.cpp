#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QByteArray"
#include <CString>
#include <QThread>
#include <QEventLoop>
#include <QPushButton>
#include "Tool/readutil.h"
#include "MyTcp/cdata.h"
using namespace  std;
MainWindow::MainWindow(SocketLink * socket,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    this->m_socket=socket;
    this->setMinimumSize(CData::current_width,CData::current_height);

    ui->setupUi(this);

    //login_widget=new LoginWidget;
    //login_widget->show();
    init_stack_widget();

    ui->treeWidget->setObjectName("treeSideMenu");
    ReadUtil::setWidgetQss(ui->treeWidget, ":/qss/tree.qss");

    QThread *thread=new QThread;
    m_socket->moveToThread(thread);
    thread->start();
    qDebug()<<"【主循环pid】"<<QThread::currentThreadId();
    init_task_connect();

    init_connect();


    // // 2.局部事件循环，等待连接结果，最多超时8秒
    // QEventLoop loop;
    // bool connect_ok = false;

    // QTimer timer;
    // timer.setSingleShot(true);
    // timer.setInterval(8000); //8秒超时

    // //连接成功
    // QObject::connect(m_socket->socket, &QTcpSocket::connected, &loop, [&](){
    //     connect_ok = true;
    //     loop.quit();
    // });
    // //连接出错
    // QObject::connect(m_socket->socket, &QTcpSocket::errorOccurred, &loop, [&](){
    //     connect_ok = false;
    //     loop.quit();
    // });
    // //超时
    // QObject::connect(&timer, &QTimer::timeout, &loop, [&](){
    //     connect_ok = false;
    //     loop.quit();
    // });

    // timer.start();
    // loop.exec(); // 这里阻塞，但是Qt事件继续跑，网络握手正常工作
    // timer.stop();



}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_connect()
{
    ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(0));
    //主到约会
    connect(sys_widget,&SysWidget::to_app_page,this,[this](){
        ui->stackedWidget->setCurrentWidget(appoint_widget);
        CData::current_widget=appoint_widget;
        ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(4));
    });
    //主到值班
    connect(sys_widget,&SysWidget::to_guard_page,this,[this](){
        ui->stackedWidget->setCurrentWidget(guard_widget);
        CData::current_widget=guard_widget;
        ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(3));
    });


    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int column){
        int index = ui->treeWidget->indexOfTopLevelItem(item);

        switch(index){
        case 0:{ui->stackedWidget->setCurrentWidget(sys_widget);CData::current_widget=sys_widget;break;}
        case 1:break;
        case 3:{ui->stackedWidget->setCurrentWidget(guard_widget);CData::current_widget=guard_widget;break;}
        case 4:{ui->stackedWidget->setCurrentWidget(appoint_widget);CData::current_widget=appoint_widget;break;}
        case 5:{ui->stackedWidget->setCurrentWidget(order_widget);CData::current_widget=order_widget;break;}
        }

    });

}

void MainWindow::init_task_connect()
{
    //connect(this->login_widget,&LoginWidget::to_login_ok,this->m_socket,&SocketLink::send_data);

    connect(this->appoint_widget,&AppointWidget::to_get_meet,this->m_socket,&SocketLink::send_data);

    //值班管理流
    connect(this->order_widget,&OrderWidget::get_doctor_info,this->m_socket,&SocketLink::send_data);

    connect(this->m_socket,&SocketLink::get_doctor_info_success,this->order_widget,&OrderWidget::flush_doctor);

    connect(this->m_socket,&SocketLink::get_guard_info_success,this->order_widget,&OrderWidget::flush_table);
    //值班查询流

    connect(this->guard_widget,&GuardWidget::send_my_data,this->m_socket,&SocketLink::send_data);

    connect(this->m_socket,&SocketLink::get_guard_info_success,this->guard_widget,&GuardWidget::flush_table);




    connect(this->m_socket,&SocketLink::login_success,this,[this](){
        //login_widget->hide();
        this->show();
    });
    //预约流
    connect(this->m_socket,&SocketLink::get_app_success,this,[this](){
        this->appoint_widget->flush();
    });


}

void MainWindow::init_stack_widget()
{
    sys_widget=new SysWidget(this);

    appoint_widget=new AppointWidget;

    guard_widget=new GuardWidget;

    order_widget=new OrderWidget;

    ui->stackedWidget->addWidget(sys_widget);

    ui->stackedWidget->addWidget(appoint_widget);

    ui->stackedWidget->addWidget(guard_widget);

    ui->stackedWidget->addWidget(order_widget);
    //设置默认页（系统页）
    ui->stackedWidget->setCurrentWidget(sys_widget);
    CData::current_widget=sys_widget;


}
