#include "syswidget.h"
#include "ui_syswidget.h"
#include "Tool/myutils.h"
SysWidget::SysWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SysWidget)
{
    ui->setupUi(this);
    init_myTable();
    this->setAttribute(Qt::WA_StyledBackground, true);
    MyUtils::setBack(this);
    init_connect();
}

SysWidget::~SysWidget()
{
    delete ui;
}

void SysWidget::init_myTable()
{
    MyUtils::setIcons(ui->toolButton,"查看病例",":/icons/sys/dangan.png");
    MyUtils::setIcons(ui->toolButton_2,"工作统计",":/icons/sys/tongji.png");
    MyUtils::setIcons(ui->toolButton_3,"查看预约",":/icons/sys/yuyue.png");
    MyUtils::setIcons(ui->toolButton_4,"值班信息",":/icons/sys/zhiban.png");
    MyUtils::setIcons(ui->toolButton_5,"查看预约",":/icons/sys/yuyue.png");
    MyUtils::setIcons(ui->toolButton_6,"值班信息",":/icons/sys/zhiban.png");
    MyUtils::setLabel(ui->label,40,true);
    MyUtils::setLabel(ui->label_2,15,false);
}

void SysWidget::init_connect()
{
    connect(ui->toolButton_3,&QToolButton::clicked,this,[this](){
        emit to_app_page();
    });
    //去值班
    connect(ui->toolButton_4,&QToolButton::clicked,this,[this](){
        emit to_guard_page();
    });
    //去工作统计
    connect(ui->toolButton_2,&QToolButton::clicked,this,[this](){
        emit to_workstat_page();
    });

}
