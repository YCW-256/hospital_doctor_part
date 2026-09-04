#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "Task/tologintask.h"
LoginWidget::LoginWidget(SocketLink *socket,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
    this->m_socket=socket;

    ui->setupUi(this);

    init_connect();
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::init_connect()
{
    ui->loginStackedWidget->setCurrentWidget(ui->pagePhone);
    connect(ui->tabInsuranceButton,&QPushButton::clicked,this,[this](){
        ui->loginStackedWidget->setCurrentWidget(ui->pageInsurance);
    });
    connect(ui->tabPhoneButton,&QPushButton::clicked,this,[this](){
        ui->loginStackedWidget->setCurrentWidget(ui->pagePhone);
    });
    connect(ui->loginButton,&QPushButton::clicked,this,[this](){
        ToLoginTask *task=new ToLoginTask(ui->insuranceEdit->text(),ui->passwordEdit->text(),ui->captchaEdit->text());
        task->execute();
        emit to_login_ok(task->send_data,task->len);
        //emit send_ok(task->send_data,task->len);
        m_socket->send_data(task->send_data,task->len);
        delete task;
    });
    //适配
    connect(this->m_socket,&SocketLink::login_success,this,[this](){
        this->hide();
        emit loginSuccess();
        //this->show();
    });

}
