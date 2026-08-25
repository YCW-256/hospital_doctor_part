#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QByteArray"
#include <CString>
#include <QThread>
#include <QEventLoop>
#include <QPushButton>
using namespace  std;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->label->setAvatar(":/icons/avert1.jpg");
    m_socket=new SocketLink;
    m_socket->connectHost();
    login_widget=new LoginWidget;
    login_widget->show();
    QThread *thread=new QThread;
    m_socket->moveToThread(thread);
    thread->start();
    qDebug()<<"【主循环pid】"<<QThread::currentThreadId();
    init_task_connect();

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

}

void MainWindow::init_task_connect()
{
    connect(this->login_widget,&LoginWidget::to_login_ok,this->m_socket,&SocketLink::send_data);
}
