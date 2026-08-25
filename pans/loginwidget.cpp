#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "Task/tologintask.h"
LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoginWidget)
{
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
        delete task;
    });

}
