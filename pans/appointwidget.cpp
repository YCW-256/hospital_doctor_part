#include "appointwidget.h"
#include "ui_appointwidget.h"
#include <QButtonGroup>
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"
#include <QScrollBar>
//按钮索引  日  周  月  0  1  2
AppointWidget::AppointWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AppointWidget)
{
    ui->setupUi(this);
    init_connect();

}

AppointWidget::~AppointWidget()
{
    delete ui;
}

void AppointWidget::flush()
{
    // 删除 widget_2
    if (ui->widget_2) {
        ui->gridLayout_3->removeWidget(ui->widget_2);
        delete ui->widget_2;
        ui->widget_2 = nullptr;
    }

    // 删除 widget_3
    if (ui->widget_3) {
        ui->gridLayout_3->removeWidget(ui->widget_3);
        delete ui->widget_3;
        ui->widget_3 = nullptr;
    }

    // 删除 widget_4
    if (ui->widget_4) {
        ui->gridLayout_3->removeWidget(ui->widget_4);
        delete ui->widget_4;
        ui->widget_4 = nullptr;
    }

    // 删除 widget_5
    if (ui->widget_5) {
        ui->gridLayout_3->removeWidget(ui->widget_5);
        delete ui->widget_5;
        ui->widget_5 = nullptr;
    }

    // 删除 widget_6
    if (ui->widget_6) {
        ui->gridLayout_3->removeWidget(ui->widget_6);
        delete ui->widget_6;
        ui->widget_6 = nullptr;
    }

    // 注意：布局 gridLayout_3 和 verticalSpacer 保留了下来。
    // 因为已置空，即使 ui 类析构时也不会重复 delete 导致崩溃。
    int row=(CData::app_info.size()+1)/2;
    for(int i=0;i<row;i++){
        for(int j=0;j<2;j++){
            QString name=CData::app_info[i].name;
            QString time=CData::app_info[i].time;
            qDebug()<<name<<time;
            int state=CData::app_info[i].state;
            MedicalCardWidget *newCard = new MedicalCardWidget(this);
            newCard->setFixedSize(QSize(380,129));
            newCard->setInfo(name, "外科", "我", time, "09:30", state, state);
            ui->gridLayout_3->addWidget(newCard, i, j);
        }
    }

}


void AppointWidget::init_connect()
{
    connect(ui->sel_btn,&SelBtn::currentIndexChanged,this,[this](int idx){
        qDebug()<<"选中索引："<<idx;
        this->getAppInfo(idx);
    });
}

void AppointWidget::getAppInfo(int idx)
{
    DOCTOR_APP_REQ req;
    req.id=CData::m_id;
    req.style=idx;
    HEAD head;
    head.is_fragment=0;
    head.len=sizeof(req);
    head.type=SERVICE_TYPE::DOCTOR_APP_INFO;
    QByteArray data;
    data.resize(1024);
    char *p=data.data();
    memcpy(p,&head,sizeof(head));
    memcpy(p+sizeof(head),&req,sizeof(req));
    emit to_get_meet(data,sizeof(head)+sizeof(req));
    qDebug()<<head.len<<"  "<<sizeof(head)<<"  "<<sizeof(head)+sizeof(req);

}
