#include "appointwidget.h"
#include "ui_appointwidget.h"
#include <QButtonGroup>
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"
#include "../Tool/myutils.h"
#include "childs/doctor/appointdetailwidget.h"
#include <QScrollBar>
#include <cstring>
//按钮索引  日  周  月  0  1  2

// 把 QString 拷进定长 char 数组（UTF-8 字节），带结束符（同 doctororder.cpp 的做法）
static void copyCStr(char *dst, int cap, const QString &s)
{
    QByteArray ba = s.toUtf8();
    int n = ba.size();
    if (n > cap - 1)
        n = cap - 1;
    if (n > 0)
        memcpy(dst, ba.constData(), n);
    dst[n] = '\0';
}

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

        // 点击该卡片 → 弹出该条预约的接诊详情
        connect(newCard, &MedicalCardWidget::cardClicked, this, [this, i]() {
            this->openMeetDetail(i);
        });

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

void AppointWidget::openMeetDetail(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(CData::app_info.size()))
        return;

    // 用当前索引把该次预约的信息做成快照传给弹窗（不引 CData 全局量的引用，
    // 弹窗运行期间即使列表被刷新，弹窗内容也不受影响）
    const APP_INFO &info = CData::app_info[idx];
    MeetRecord rec;
    rec.meet_id     = info.meet_id;
    rec.doctor_id   = CData::m_id;   // 登录医生即是接诊医生（查的是自己的预约）
    rec.patient_id  = info.patient_id;
    rec.doctor_name = info.doctorName;
    rec.patient_name= info.name;
    rec.time        = info.time;

    AppointDetailWidget dlg(rec, this);
    // 只有点“完成”并二次确认过才返回 Accepted，此时才发 DOCTOR_SET_RECORD
    if (dlg.exec() != QDialog::Accepted)
        return;
    sendRecord(rec, dlg.diagnosis(), dlg.treatPlan());
}

void AppointWidget::sendRecord(const MeetRecord &rec,
                               const QString &diagnosis,
                               const QString &treatPlan)
{
    HEAD head;
    memset(&head, 0, sizeof(head));
    head.is_fragment = 0;
    head.len = sizeof(SET_RECORD_REQ);   // len = body 大小
    head.type = SERVICE_TYPE::DOCTOR_SET_RECORD;

    SET_RECORD_REQ req;
    memset(&req, 0, sizeof(req));
    req.meet_id    = rec.meet_id;
    req.doctor_id  = rec.doctor_id;
    req.patient_id = rec.patient_id;
    copyCStr(req.diagnosis,  sizeof(req.diagnosis),  diagnosis);
    copyCStr(req.treat_plan, sizeof(req.treat_plan), treatPlan);

    int send_size = sizeof(head) + sizeof(req);
    QByteArray data;
    data.resize(send_size);
    char *p = data.data();
    memcpy(p, &head, sizeof(head));
    memcpy(p + sizeof(head), &req, sizeof(req));

    qDebug() << "发送就诊记录 DOCTOR_SET_RECORD: meet" << rec.meet_id
             << "doctor" << rec.doctor_id << "patient" << rec.patient_id
             << "| diagnosis:" << diagnosis << "| treat_plan:" << treatPlan;
    emit to_get_meet(data, send_size);
}
