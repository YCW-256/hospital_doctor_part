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
    // “获得图片”：弹窗还开着时就要发出去（不等弹窗关闭），这里直连到组包函数。
    // 医生 id / 患者 id 直接取快照 rec（与弹窗发来的两个参数同源），故 lambda 只用到 date。
    // 注意：Qt6 的 connect 不接受“参数比信号少”的 lambda（会报 C2039 QtPrivate::value/FunctorReturnType
    // 一串看不懂的模板错），所以按信号原样写全 3 个参数，未用的两个 Q_UNUSED 掉。
    connect(&dlg, &AppointDetailWidget::to_get_tongue_img, this,
            [this, rec](int doctorId, int patientId, const QString &date) {
                Q_UNUSED(doctorId);
                Q_UNUSED(patientId);
                sendGetTongueImg(rec, date);
            });

    // 记下当前弹窗：图片是异步回包（分片拼齐才发信号），到时靠这个指针把图转给弹窗显示。
    // 弹窗是栈对象，exec() 一返回就析构 —— 必须马上置空，否则后面到的回包会踩悬空指针。
    m_detailDlg = &dlg;
    const int ret = dlg.exec();
    m_detailDlg = nullptr;

    // 只有点“完成”并二次确认过才返回 Accepted，此时才发 DOCTOR_SET_RECORD
    if (ret != QDialog::Accepted)
        return;
    sendRecord(rec, dlg.diagnosis(), dlg.treatPlan());
}

void AppointWidget::flush_tongue_img()
{
    // 弹窗没开就没地方显示，忽略即可（图片已经落在 CData 里，下次开窗 showTonguePixmap() 会直接显示）
    if (!m_detailDlg)
        return;
    m_detailDlg->setTongueImage(CData::tongue_image, CData::tongue_image_patient_id);
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

void AppointWidget::sendGetTongueImg(const MeetRecord &rec, const QString &date)
{
    // 请求舌苔图片：医生 id + 患者 id + 日期（年月日），打包规范同 getAppInfo/sendRecord。
    // 回包是同一 type 的若干 IMG_T 分片（见 Task/gettongueimgtask），收齐后经
    // SocketLink::get_tongue_img_success → AppointWidget::flush_tongue_img 转给弹窗显示。
    HEAD head;
    memset(&head, 0, sizeof(head));
    head.is_fragment = 0;
    head.len = sizeof(GET_TONGUE_IMG_REQ);   // len = body 大小
    head.type = SERVICE_TYPE::GET_TONGUE_IMG;

    GET_TONGUE_IMG_REQ req;
    memset(&req, 0, sizeof(req));
    req.doctor_id  = rec.doctor_id;
    req.patient_id = rec.patient_id;
    copyCStr(req.date, sizeof(req.date), date);

    int send_size = sizeof(head) + sizeof(req);
    QByteArray data;
    data.resize(send_size);
    char *p = data.data();
    memcpy(p, &head, sizeof(head));
    memcpy(p + sizeof(head), &req, sizeof(req));

    qDebug() << "发送舌苔图片请求 GET_TONGUE_IMG: doctor" << req.doctor_id
             << "patient" << req.patient_id << "date" << date;
    emit to_get_meet(data, send_size);
}
