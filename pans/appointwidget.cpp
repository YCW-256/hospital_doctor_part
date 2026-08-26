#include "appointwidget.h"
#include "ui_appointwidget.h"
#include <QButtonGroup>
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"
#include "../Tool/myutils.h"
#include <QScrollBar>
//按钮索引  日  周  月  0  1  2
AppointWidget::AppointWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AppointWidget)
{

    ui->setupUi(this);
    ui->scrollArea->setStyleSheet("background: transparent;");
    ui->scrollArea->viewport()->setStyleSheet("background: transparent;");
    ui->label->setStyleSheet("background: transparent;");
    init_connect();
    MyUtils::setBack(this,"#E8F2F6");
    this->setAttribute(Qt::WA_StyledBackground, true);
}

AppointWidget::~AppointWidget()
{
    delete ui;
}

void AppointWidget::flush()
{
    // 1. 动态清空布局内所有项（包括初始的widget_2~6，以及之前动态添加的所有卡片）
    // 使用 takeAt(0) 每次取出第一个项，直到取完
    QLayoutItem *child;
    while ((child = ui->gridLayout_3->takeAt(0)) != 0) {
        if (child->widget()) {
            // 删除 widget（MedicalCardWidget）
            delete child->widget();
        }
        // 删除 layout item 本身（如果是 Spacer，也一并清理）
        delete child;
    }

    // 2. 如果你保留了 verticalSpacer，但在上面被删除了，可以重新加回去：
    // 或者直接使用 setRowStretch 让内容顶端对齐（推荐）
    // ui->gridLayout_3->setRowStretch(ui->gridLayout_3->rowCount(), 1);

    // 3. 重新添加数据项
    int count = CData::app_info.size();
    for (int i = 0; i < count; ++i) {
        // 取全局第 i 条数据
        QString name = CData::app_info[i].name;
        QString time = CData::app_info[i].time;
        int state = CData::app_info[i].state;

        MedicalCardWidget *newCard = new MedicalCardWidget(this);
        newCard->setFixedSize(QSize(430, 129));

        // 注意：因为去掉了 time2，setInfo 现在只有 6 个参数
        // 且最后的两个参数是 bool 类型，如果 state 有具体的枚举值，请自行转化为布尔
        bool hasVisited = (state == 1); // 假设1是已就诊
        bool confirmed = (state == 2);  // 假设2是已确认

        newCard->setInfo(name, "外科", "我", time, hasVisited, confirmed);

        // 计算行列：每行放2个
        int row = i / 2;
        int col = i % 2;

        ui->gridLayout_3->addWidget(newCard, row, col);
    }

    // 确保底部撑开
    ui->gridLayout_3->setRowStretch(ui->gridLayout_3->rowCount(), 1);
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
