#include "recordwidget.h"

#include <QAbstractSpinBox>
#include <QColor>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <cstring>

#include "../MyTcp/cdata.h"
#include "../MyTcp/protecol.h"

namespace {
// 列表列：0 就诊日期 | 1 姓名 | 2 患者编号 | 3 主要症状 | 4 状态；第 0 列额外用 UserRole 存它在 m_rows 里的下标
const int kColCount = 5;
const Qt::ItemDataRole kRowIndexRole = Qt::UserRole;

// 查询条 / 表单里的小标签
QLabel *makeLabel(QWidget *parent, const QString &text)
{
    QLabel *l = new QLabel(text, parent);
    l->setStyleSheet("background: transparent; border:none; font-size:14px; color:#4A5A6A;");
    return l;
}

// 白色圆角卡片（和工作统计页一致）
QFrame *makeCard(QWidget *parent)
{
    QFrame *card = new QFrame(parent);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet("QFrame { background-color:#FFFFFF; border:1px solid #D9E7F5; border-radius:12px; }");
    return card;
}

// 主按钮 / 次按钮（浅蓝医疗风的两种按钮样式）
void stylePrimary(QPushButton *b)
{
    b->setCursor(Qt::PointingHandCursor);
    b->setMinimumSize(92, 34);
    b->setStyleSheet(
        "QPushButton{ background:#2F80ED; color:#FFFFFF; border:none; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#1E70BF; }"
        "QPushButton:disabled{ background:#C7D8EA; }");
}

void styleSecondary(QPushButton *b)
{
    b->setCursor(Qt::PointingHandCursor);
    b->setMinimumSize(92, 34);
    b->setStyleSheet(
        "QPushButton{ background:#E6F0FA; color:#333333; border:1px solid #B7D4F2; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#D6E6F5; }"
        "QPushButton:disabled{ color:#AAB4BE; }");
}

// 输入框统一样式（只读态单独一种底色，一眼能看出哪些不让改）
const char *kEditQss =
    "QLineEdit, QDateEdit, QComboBox, QSpinBox{"
    " background:#FFFFFF; border:1px solid #B7D4F2; border-radius:6px;"
    " padding:5px 8px; font-size:14px; color:#333333; min-height:22px; }"
    "QLineEdit:focus, QDateEdit:focus, QComboBox:focus, QSpinBox:focus{ border:1px solid #2F80ED; }"
    "QLineEdit:disabled, QDateEdit:disabled, QComboBox:disabled, QSpinBox:disabled{"
    " background:#F2F6FA; border:1px solid #E1EAF3; color:#6B7B8C; }";

const char *kMultiEditQss =
    "QTextEdit{ background:#FFFFFF; border:1px solid #B7D4F2; border-radius:6px;"
    " padding:6px; font-size:14px; color:#333333; }"
    "QTextEdit:focus{ border:1px solid #2F80ED; }"
    "QTextEdit:disabled{ background:#F2F6FA; border:1px solid #E1EAF3; color:#6B7B8C; }";

// 日期编辑框：**不要下拉箭头**（QDateEdit 的上下/弹出箭头一律去掉，外观和普通输入框一致），
// 日期直接键入 yyyy-MM-dd；想改日期可用键盘上下键微调。
QDateEdit *makeDateEdit(QWidget *parent, const QDate &date)
{
    QDateEdit *d = new QDateEdit(date, parent);
    d->setButtonSymbols(QAbstractSpinBox::NoButtons);   // 去掉箭头按钮（日历弹出按钮也是它）
    d->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    d->setStyleSheet(kEditQss);
    return d;
}

// 只读输入框（患者信息这类不给改的字段）
QLineEdit *makeReadOnlyEdit(QWidget *parent)
{
    QLineEdit *e = new QLineEdit(parent);
    e->setReadOnly(true);
    e->setStyleSheet(kEditQss);
    return e;
}

// 把 QString 拷进定长 char 数组（UTF-8 字节），带结束符（同 appointwidget.cpp 的做法）
void copyCStr(char *dst, int cap, const QString &s)
{
    QByteArray ba = s.toUtf8();
    int n = ba.size();
    if (n > cap - 1)
        n = cap - 1;
    if (n > 0)
        memcpy(dst, ba.constData(), n);
    dst[n] = '\0';
}
} // namespace

RecordWidget::RecordWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentRow(-1)
    , m_loading(false)
    , m_serverTotal(-1)
    , m_pendingDetailId(-1)
    , m_nameEdit(nullptr)
    , m_dateFrom(nullptr)
    , m_dateTo(nullptr)
    , m_queryBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_table(nullptr)
    , m_countLabel(nullptr)
    , m_formTip(nullptr)
    , m_fDate(nullptr)
    , m_fName(nullptr)
    , m_fPatientId(nullptr)
    , m_fSex(nullptr)
    , m_fAge(nullptr)
    , m_fDoctor(nullptr)
    , m_fSymptom(nullptr)
    , m_fDiagnosis(nullptr)
    , m_fTreatPlan(nullptr)
    , m_saveBtn(nullptr)
    , m_revertBtn(nullptr)
{
    buildUi();
    // 只建界面，不造数据：列表等医生点“查询”发 GET_MEDICAL_RECORD、回包后由 flush_table() 灌
    refreshTable();
}

void RecordWidget::buildUi()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("RecordWidget { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                  " stop:0 #E8F2F6, stop:1 #FFFFFF); }");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    QLabel *title = new QLabel(QStringLiteral("查看病例"), this);
    title->setStyleSheet("background: transparent; border:none; font-size:20px;"
                         " font-weight:bold; color:#1E70BF;");
    root->addWidget(title);

    QLabel *sub = new QLabel(QStringLiteral("按姓名 / 就诊日期查询列表，选中某条后再取该条的完整病历"), this);
    sub->setStyleSheet("background: transparent; border:none; font-size:13px; color:#8A9AB0;");
    root->addWidget(sub);

    buildQueryBar(root);

    // ---- 主体：左侧列表 + 右侧详情表单 ----
    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(14);
    buildTable(this, body);
    buildForm(this, body);
    root->addLayout(body, 1);

    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet("background: transparent; border:none; font-size:13px; color:#8A9AB0;");
    root->addWidget(m_countLabel);

    // ---- 信号槽 ----
    connect(m_queryBtn, &QPushButton::clicked, this, &RecordWidget::onQuery);
    connect(m_resetBtn, &QPushButton::clicked, this, &RecordWidget::onReset);
    connect(m_saveBtn, &QPushButton::clicked, this, &RecordWidget::onSave);
    connect(m_revertBtn, &QPushButton::clicked, this, &RecordWidget::onRevert);
    // 选中行变化（用 currentCellChanged：itemSelectionChanged 在刷新表格时会反复触发）
    connect(m_table, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) { Q_UNUSED(row); onRowChanged(); });
    // 姓名框回车 = 点查询；两个日期框改完也顺手查一次
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &RecordWidget::onQuery);
    // 改日期只按本地已有结果重筛（不重发请求，免得每敲一位数字就发一包）；
    // m_serverTotal 作废——它是上一次服务端查询的总数，本地重筛后再报它就不准了
    connect(m_dateFrom, &QDateEdit::dateChanged, this, [this](const QDate &) {
        m_serverTotal = -1;
        refreshTable();
    });
    connect(m_dateTo, &QDateEdit::dateChanged, this, [this](const QDate &) {
        m_serverTotal = -1;
        refreshTable();
    });

    setDetailEnabled(false);   // 未选中记录 / 详情未到时，详情字段先置灰
    m_formTip->clear();        // 还没选记录，别显示“详情加载中…”
}

void RecordWidget::buildQueryBar(QVBoxLayout *root)
{
    QFrame *card = makeCard(this);
    QHBoxLayout *bar = new QHBoxLayout(card);
    bar->setContentsMargins(16, 12, 16, 12);
    bar->setSpacing(10);

    // 默认查本月的记录
    const QDate today = QDate::currentDate();
    const QDate monthBegin(today.year(), today.month(), 1);

    bar->addWidget(makeLabel(card, QStringLiteral("姓名")));
    m_nameEdit = new QLineEdit(card);
    m_nameEdit->setPlaceholderText(QStringLiteral("患者姓名（留空为全部）"));
    m_nameEdit->setFixedWidth(170);
    m_nameEdit->setStyleSheet(kEditQss);
    bar->addWidget(m_nameEdit);

    bar->addSpacing(6);
    bar->addWidget(makeLabel(card, QStringLiteral("就诊日期")));
    m_dateFrom = makeDateEdit(card, monthBegin);
    bar->addWidget(m_dateFrom);
    bar->addWidget(makeLabel(card, QStringLiteral("至")));
    m_dateTo = makeDateEdit(card, today);
    bar->addWidget(m_dateTo);

    bar->addStretch();

    m_queryBtn = new QPushButton(QStringLiteral("查询"), card);
    m_resetBtn = new QPushButton(QStringLiteral("重置"), card);
    stylePrimary(m_queryBtn);
    styleSecondary(m_resetBtn);
    bar->addWidget(m_queryBtn);
    bar->addWidget(m_resetBtn);

    root->addWidget(card);
}

void RecordWidget::buildTable(QWidget *parent, QHBoxLayout *row)
{
    QFrame *card = makeCard(parent);
    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(8);

    QLabel *cap = new QLabel(QStringLiteral("病历列表"), card);
    cap->setStyleSheet("background: transparent; border:none; font-size:15px;"
                       " font-weight:bold; color:#1E70BF;");
    lay->addWidget(cap);

    m_table = new QTableWidget(card);
    m_table->setColumnCount(kColCount);
    m_table->setHorizontalHeaderLabels({QStringLiteral("就诊日期"), QStringLiteral("姓名"),
                                        QStringLiteral("患者编号"), QStringLiteral("主要症状"),
                                        QStringLiteral("状态")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);   // 改内容走右侧表单
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setShowGrid(true);
    m_table->setStyleSheet(
        "QTableWidget{ background:#FFFFFF; border:1px solid #D9E7F5; gridline-color:#E6EFF8; font-size:14px; }"
        "QHeaderView::section{ background:#EAF3FB; color:#1E70BF; font-weight:bold; font-size:14px;"
        " border:none; border-bottom:1px solid #D9E7F5; padding:8px; }"
        "QTableWidget::item{ padding:2px 6px; }"
        "QTableWidget::item:selected{ background:#D6E6F5; color:#1E70BF; }");
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setMinimumHeight(36);
    lay->addWidget(m_table, 1);

    row->addWidget(card, 3);
}

void RecordWidget::buildForm(QWidget *parent, QHBoxLayout *row)
{
    QFrame *card = makeCard(parent);
    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(16, 12, 16, 14);
    lay->setSpacing(8);

    // 标题行：左“病历详情（可修改）”，右提示（详情加载中 / 已加载）
    QHBoxLayout *capRow = new QHBoxLayout;
    QLabel *cap = new QLabel(QStringLiteral("病历详情（可修改）"), card);
    cap->setStyleSheet("background: transparent; border:none; font-size:15px;"
                       " font-weight:bold; color:#1E70BF;");
    capRow->addWidget(cap);
    capRow->addStretch();
    m_formTip = new QLabel(card);
    m_formTip->setStyleSheet("background: transparent; border:none; font-size:12px; color:#8A9AB0;");
    capRow->addWidget(m_formTip);
    lay->addLayout(capRow);

    // 一行两列的小字段：左标签 + 右控件
    auto addField = [&](const QString &text, QWidget *w) {
        QHBoxLayout *h = new QHBoxLayout;
        h->setSpacing(8);
        QLabel *l = makeLabel(card, text);
        l->setFixedWidth(70);
        h->addWidget(l);
        h->addWidget(w, 1);
        lay->addLayout(h);
    };

    // ---- 患者信息（来自列表/详情，只读）----
    m_fName = makeReadOnlyEdit(card);
    addField(QStringLiteral("姓名"), m_fName);

    m_fPatientId = makeReadOnlyEdit(card);
    addField(QStringLiteral("患者编号"), m_fPatientId);

    m_fSex = new QComboBox(card);
    m_fSex->addItems({QStringLiteral("男"), QStringLiteral("女")});
    m_fSex->setEnabled(false);           // 只读展示
    m_fSex->setStyleSheet(kEditQss);
    addField(QStringLiteral("性别"), m_fSex);

    m_fAge = new QSpinBox(card);
    m_fAge->setRange(0, 150);
    m_fAge->setEnabled(false);           // 只读展示
    m_fAge->setStyleSheet(kEditQss);
    addField(QStringLiteral("年龄"), m_fAge);

    m_fDoctor = makeReadOnlyEdit(card);
    addField(QStringLiteral("接诊医生"), m_fDoctor);

    // ---- 病历内容（可改）----
    m_fDate = makeDateEdit(card, QDate::currentDate());
    addField(QStringLiteral("就诊日期"), m_fDate);

    QLabel *symCap = makeLabel(card, QStringLiteral("主要症状"));
    lay->addWidget(symCap);
    m_fSymptom = new QTextEdit(card);
    m_fSymptom->setPlaceholderText(QStringLiteral("请输入主要症状……"));
    m_fSymptom->setFixedHeight(60);
    m_fSymptom->setStyleSheet(kMultiEditQss);
    lay->addWidget(m_fSymptom);

    QLabel *diagCap = makeLabel(card, QStringLiteral("诊断"));
    lay->addWidget(diagCap);
    m_fDiagnosis = new QTextEdit(card);
    m_fDiagnosis->setPlaceholderText(QStringLiteral("请输入诊断……"));
    m_fDiagnosis->setFixedHeight(70);
    m_fDiagnosis->setStyleSheet(kMultiEditQss);
    lay->addWidget(m_fDiagnosis);

    QLabel *treatCap = makeLabel(card, QStringLiteral("治疗意见（处方）"));
    lay->addWidget(treatCap);
    m_fTreatPlan = new QTextEdit(card);
    m_fTreatPlan->setPlaceholderText(QStringLiteral("请输入治疗意见 / 处方……"));
    m_fTreatPlan->setFixedHeight(90);
    m_fTreatPlan->setStyleSheet(kMultiEditQss);
    lay->addWidget(m_fTreatPlan);

    lay->addStretch();

    QHBoxLayout *btns = new QHBoxLayout;
    btns->addStretch();
    m_revertBtn = new QPushButton(QStringLiteral("撤销修改"), card);
    m_saveBtn = new QPushButton(QStringLiteral("保存修改"), card);
    styleSecondary(m_revertBtn);
    stylePrimary(m_saveBtn);
    btns->addWidget(m_revertBtn);
    btns->addWidget(m_saveBtn);
    lay->addLayout(btns);

    row->addWidget(card, 2);
}

//--------------------------------------------------------------- 组包（发送侧）

QByteArray RecordWidget::makeQueryPack(int *outSize) const
{
    // 第一套：按医生 id 查列表（姓名/日期区间是可选过滤，留空 = 不限）
    MEDICAL_RECORD_REQ req;
    memset(&req, 0, sizeof(req));
    req.doctor_id = CData::m_id;   // 查询主键：当前登录医生自己的病历
    req.state     = -1;            // 状态不限（正常/作废都查回来，列表里标出来）
    copyCStr(req.patient_name, sizeof(req.patient_name), m_nameEdit->text().trimmed());
    copyCStr(req.date_begin, sizeof(req.date_begin),
             m_dateFrom->date().toString(QStringLiteral("yyyy-MM-dd")));
    copyCStr(req.date_end, sizeof(req.date_end),
             m_dateTo->date().toString(QStringLiteral("yyyy-MM-dd")));

    HEAD head;
    memset(&head, 0, sizeof(head));
    head.is_fragment = 0;
    head.len  = sizeof(MEDICAL_RECORD_REQ);   // len = body 字节数
    head.type = SERVICE_TYPE::GET_MEDICAL_RECORD;

    const int total = sizeof(HEAD) + sizeof(MEDICAL_RECORD_REQ);
    QByteArray data;
    data.resize(total);   // 只按实际长度开，别学 appointwidget 的 resize(1024) 再截断
    char *p = data.data();
    memcpy(p, &head, sizeof(HEAD));
    memcpy(p + sizeof(HEAD), &req, sizeof(MEDICAL_RECORD_REQ));

    if (outSize)
        *outSize = total;
    return data;
}

QByteArray RecordWidget::makeDetailPack(int recordId, int *outSize) const
{
    // 第二套：选中某条后按 record_id 取详情
    MEDICAL_RECORD_DETAIL_REQ req;
    memset(&req, 0, sizeof(req));
    req.record_id = recordId;
    req.doctor_id = CData::m_id;

    HEAD head;
    memset(&head, 0, sizeof(head));
    head.is_fragment = 0;
    head.len  = sizeof(MEDICAL_RECORD_DETAIL_REQ);
    head.type = SERVICE_TYPE::GET_MEDICAL_RECORD_DETAIL;

    const int total = sizeof(HEAD) + sizeof(MEDICAL_RECORD_DETAIL_REQ);
    QByteArray data;
    data.resize(total);
    char *p = data.data();
    memcpy(p, &head, sizeof(HEAD));
    memcpy(p + sizeof(HEAD), &req, sizeof(MEDICAL_RECORD_DETAIL_REQ));

    if (outSize)
        *outSize = total;
    return data;
}

QByteArray RecordWidget::makeSavePack(const RecordDetail &d, int *outSize) const
{
    // 保存修改 = 把“新增病例”那套包原样再发一遍（对照 appointwidget::sendRecord 逐字段一致）：
    // HEAD(type=DOCTOR_SET_RECORD) + SET_RECORD_REQ{meet_id/doctor_id/patient_id/diagnosis/treat_plan}。
    // 医生只能新增/更新病例，不能删除。
    HEAD head;
    memset(&head, 0, sizeof(head));
    head.is_fragment = 0;
    head.len  = sizeof(SET_RECORD_REQ);   // len = body 大小
    head.type = SERVICE_TYPE::DOCTOR_SET_RECORD;

    SET_RECORD_REQ req;
    memset(&req, 0, sizeof(req));
    req.meet_id    = 0;   // 本页只保存修改、不关联预约，服务端也用不上这个字段，固定填 0
    // 本页查的就是“按 CData::m_id 查自己的病历”，所以登录医生 = 这条记录的接诊医生，同 appointwidget
    req.doctor_id  = CData::m_id;
    req.patient_id = d.patientId;
    copyCStr(req.diagnosis,  sizeof(req.diagnosis),  d.diagnosis);
    copyCStr(req.treat_plan, sizeof(req.treat_plan), d.treatPlan);

    const int total = sizeof(HEAD) + sizeof(SET_RECORD_REQ);
    QByteArray data;
    data.resize(total);
    char *p = data.data();
    memcpy(p, &head, sizeof(HEAD));
    memcpy(p + sizeof(HEAD), &req, sizeof(SET_RECORD_REQ));

    if (outSize)
        *outSize = total;
    return data;
}

//--------------------------------------------------------------- 界面联动

bool RecordWidget::matchFilter(const RecordRow &r) const
{
    const QString name = m_nameEdit->text().trimmed();
    if (!name.isEmpty() && !r.name.contains(name, Qt::CaseInsensitive))
        return false;

    // 就诊日期只在“起 ~ 止”之间（record_time 以 yyyy-MM-dd 开头，字符串比较成立）
    const QString day  = r.visitDate.left(10);
    const QString from = m_dateFrom->date().toString(QStringLiteral("yyyy-MM-dd"));
    const QString to   = m_dateTo->date().toString(QStringLiteral("yyyy-MM-dd"));
    return day >= from && day <= to;
}

void RecordWidget::refreshTable()
{
    // 重刷表格会触发 currentCellChanged，用 m_loading 挡住 onRowChanged
    const bool prevLoading = m_loading;
    m_loading = true;

    m_table->setRowCount(0);
    int hit = 0;
    for (int i = 0; i < m_rows.size(); ++i) {
        const RecordRow &r = m_rows.at(i);
        if (!matchFilter(r))
            continue;

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        const QStringList texts = {r.visitDate.left(16), r.name, QString::number(r.patientId),
                                   r.mainSymptom, r.state == 0 ? QStringLiteral("正常")
                                                               : QStringLiteral("作废")};
        for (int c = 0; c < kColCount; ++c) {
            QTableWidgetItem *item = new QTableWidgetItem(texts.at(c));
            if (c == 0)
                item->setData(kRowIndexRole, i);   // 记住它在 m_rows 里的下标（表格行 ≠ 数据下标）
            if (r.state != 0)
                item->setForeground(QColor(QStringLiteral("#A0A8B0")));   // 作废的记录整行压暗
            m_table->setItem(row, c, item);
        }
        ++hit;
    }
    // 服务端给了总条数就一并报出来（本地按日期再筛过的话，条数可能少于总数）
    if (m_serverTotal >= 0)
        m_countLabel->setText(QStringLiteral("共 %1 条记录（当前显示 %2 条）").arg(m_serverTotal).arg(hit));
    else
        m_countLabel->setText(QStringLiteral("共 %1 条记录").arg(hit));

    // 原来选中的记录如果还在结果里就保持选中，否则清空右侧表单
    int keep = -1;
    if (m_currentRow >= 0) {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->item(row, 0)->data(kRowIndexRole).toInt() == m_currentRow) {
                keep = row;
                break;
            }
        }
    }
    if (keep >= 0) {
        m_table->setCurrentCell(keep, 0);
        // 同一行还是选中状态：表单没改过才重新灌（有未保存改动就留着，别让一次“查询”把医生敲的字吞掉）
        if (!formDiffersFromDetail()) {
            if (const RecordDetail *d = currentDetail())
                fillFormFromDetail(*d);
        }
    } else {
        m_currentRow = -1;
        setDetailEnabled(false);
        m_formTip->clear();   // 没选中任何记录
    }

    m_loading = prevLoading;
}

int RecordWidget::selectedDataRow() const
{
    const int row = m_table->currentRow();
    if (row < 0 || !m_table->item(row, 0))
        return -1;
    const int idx = m_table->item(row, 0)->data(kRowIndexRole).toInt();
    return (idx >= 0 && idx < m_rows.size()) ? idx : -1;
}

const RecordDetail *RecordWidget::currentDetail() const
{
    if (m_currentRow < 0 || m_currentRow >= m_rows.size())
        return nullptr;
    const int id = m_rows.at(m_currentRow).recordId;
    const auto it = m_details.constFind(id);
    return it == m_details.constEnd() ? nullptr : &it.value();
}

RecordDetail RecordWidget::detailFromRow(const RecordRow &r) const
{
    // 列表字段拼出来的“详情壳”：诊断/治疗意见还没有（等第二套回包），
    // 用来判断“等详情期间表单有没有被改过”——两边同样字段同样值就算没动过。
    RecordDetail d;
    d.recordId    = r.recordId;
    d.patientId   = r.patientId;
    d.state       = r.state;
    d.name        = r.name;
    d.visitDate   = r.visitDate;
    d.mainSymptom = r.mainSymptom;
    return d;
}

void RecordWidget::requestDetailIfNeeded()
{
    if (m_currentRow < 0 || m_currentRow >= m_rows.size())
        return;

    const RecordRow &r = m_rows.at(m_currentRow);

    // 本地缓存（或之前查过）已有这条的详情 → 直接显示，不再麻烦服务端
    if (const RecordDetail *d = currentDetail()) {
        fillFormFromDetail(*d);
        return;
    }

    // 记下“正在等哪条”和“发请求时表单长什么样”，回包时才知道要不要覆盖医生敲的字
    m_pendingDetailId = r.recordId;
    m_pendingBase     = detailFromRow(r);
    setDetailEnabled(false);   // 详情没到，先置灰 + 提示“详情加载中…”

    int size = 0;
    const QByteArray pack = makeDetailPack(r.recordId, &size);
    qDebug().noquote() << QStringLiteral("[查看病例] 选中第 %1 条（record_id=%2）→ 发送 GET_MEDICAL_RECORD_DETAIL，包长 %3 字节")
                              .arg(m_currentRow)
                              .arg(r.recordId)
                              .arg(size);
    emit to_record_detail(pack, size);
}

void RecordWidget::fillFormFromRow(const RecordRow &r)
{
    const bool prevLoading = m_loading;
    m_loading = true;   // 灌数据期间不判脏

    m_fName->setText(r.name);
    m_fPatientId->setText(r.patientId >= 0 ? QString::number(r.patientId) : QString());
    m_fDate->setDate(QDate::fromString(r.visitDate.left(10), QStringLiteral("yyyy-MM-dd")));
    m_fSymptom->setPlainText(r.mainSymptom);
    // 详情字段先清空，等第二套回来再填
    m_fSex->setCurrentIndex(0);
    m_fAge->setValue(0);
    m_fDoctor->clear();
    m_fDiagnosis->clear();
    m_fTreatPlan->clear();

    m_loading = prevLoading;
}

void RecordWidget::fillFormFromDetail(const RecordDetail &d)
{
    const bool prevLoading = m_loading;
    m_loading = true;

    m_fName->setText(d.name);
    m_fPatientId->setText(d.patientId >= 0 ? QString::number(d.patientId) : QString());
    const int sexIdx = m_fSex->findText(d.sex);
    m_fSex->setCurrentIndex(sexIdx >= 0 ? sexIdx : 0);
    m_fAge->setValue(d.age);
    m_fDoctor->setText(d.doctor);
    m_fDate->setDate(QDate::fromString(d.visitDate.left(10), QStringLiteral("yyyy-MM-dd")));
    m_fSymptom->setPlainText(d.mainSymptom);
    m_fDiagnosis->setPlainText(d.diagnosis);
    m_fTreatPlan->setPlainText(d.treatPlan);

    m_loading = prevLoading;

    setDetailEnabled(true);
    m_formTip->clear();
}

void RecordWidget::setDetailEnabled(bool on)
{
    // 详情能改的只有病历内容；患者姓名/编号/性别/年龄/接诊医生始终只读
    m_fDate->setEnabled(on);
    m_fSymptom->setEnabled(on);
    m_fDiagnosis->setEnabled(on);
    m_fTreatPlan->setEnabled(on);
    m_saveBtn->setEnabled(on);
    m_revertBtn->setEnabled(on);
    m_formTip->setText(on ? QString() : QStringLiteral("详情加载中…"));
}

void RecordWidget::readFormInto(RecordDetail &d) const
{
    // 表单只能改到“天”，时分秒沿用原记录
    QString day = m_fDate->date().toString(QStringLiteral("yyyy-MM-dd"));
    if (m_currentRow >= 0 && m_currentRow < m_rows.size())
        day += m_rows.at(m_currentRow).visitDate.mid(10);
    d.visitDate   = day;
    d.mainSymptom = m_fSymptom->toPlainText().trimmed();
    d.diagnosis   = m_fDiagnosis->toPlainText().trimmed();
    d.treatPlan   = m_fTreatPlan->toPlainText().trimmed();
}

bool RecordWidget::formDirty() const
{
    return !m_loading && formDiffersFromDetail();
}

bool RecordWidget::formDiffersFromDetail() const
{
    const RecordDetail *d = currentDetail();
    if (!d)
        return false;   // 详情还没到，表单里只有列表字段，不算“改过”
    return formDiffersFrom(*d);
}

// 纯比较：把表单“能改的那几项”读出来，跟 base 比。不判 m_loading，也不看有没有选中行
// （调用方自己决定语义）。refreshTable / flush_detail 靠它避免把医生敲的字冲掉。
bool RecordWidget::formDiffersFrom(const RecordDetail &base) const
{
    RecordDetail cur = base;    // 以 base 为底，只覆盖表单能改的那几项
    readFormInto(cur);
    return cur.visitDate != base.visitDate || cur.mainSymptom != base.mainSymptom
           || cur.diagnosis != base.diagnosis || cur.treatPlan != base.treatPlan;
}

void RecordWidget::onQuery()
{
    const QString name = m_nameEdit->text().trimmed();
    const QString from = m_dateFrom->date().toString(QStringLiteral("yyyy-MM-dd"));
    const QString to   = m_dateTo->date().toString(QStringLiteral("yyyy-MM-dd"));
    qDebug().noquote() << QStringLiteral("[查看病例] 查询：姓名=%1  日期 %2 ~ %3")
                              .arg(name.isEmpty() ? QStringLiteral("(全部)") : name, from, to);

    // 第一套：组包 → 信号 → 窗口接到 SocketLink::send_data（跨线程，走队列信号）
    int size = 0;
    const QByteArray pack = makeQueryPack(&size);
    qDebug().noquote() << QStringLiteral("[查看病例] 发送 GET_MEDICAL_RECORD：doctor_id=%1 姓名=%2 %3~%4 包长 %5 字节")
                              .arg(CData::m_id)
                              .arg(name.isEmpty() ? QStringLiteral("(不限)") : name)
                              .arg(from, to)
                              .arg(size);
    emit to_query_record(pack, size);

    // 等服务端回包：flush_table() 到了再刷列表（不再拿本地旧数据先刷一遍，免得闪一下过期内容）。
    // 这里只是把“共 N 条”改成进度提示，不禁用查询按钮——万一服务端没实现，页面还能再点。
    m_serverTotal = -1;
    m_countLabel->setText(QStringLiteral("查询中…"));
}

void RecordWidget::onReset()
{
    const QDate today = QDate::currentDate();

    const bool prevLoading = m_loading;
    m_loading = true;
    m_nameEdit->clear();
    m_dateFrom->setDate(QDate(today.year(), today.month(), 1));
    m_dateTo->setDate(today);
    m_loading = prevLoading;

    m_currentRow = -1;
    m_pendingDetailId = -1;
    onQuery();   // 重置 = 清条件后重新向服务端查一次（本地 m_rows 会被回包整体替换）
}

void RecordWidget::onRowChanged()
{
    if (m_loading)
        return;

    const int idx = selectedDataRow();

    // 换行前若表单改过还没保存，先问一句，选“否”就退回原来那行
    if (m_currentRow >= 0 && idx != m_currentRow && formDirty()) {
        const int ret = QMessageBox::question(
            this, QStringLiteral("未保存的修改"),
            QStringLiteral("当前记录的修改尚未保存，是否放弃？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            // 恢复选中到原来那行（期间屏蔽信号，避免递归）
            for (int row = 0; row < m_table->rowCount(); ++row) {
                if (m_table->item(row, 0)->data(kRowIndexRole).toInt() == m_currentRow) {
                    m_table->blockSignals(true);
                    m_table->setCurrentCell(row, 0);
                    m_table->blockSignals(false);
                    break;
                }
            }
            return;
        }
    }

    m_currentRow = idx;
    if (m_currentRow < 0) {
        setDetailEnabled(false);
        m_formTip->clear();
        return;
    }

    // ① 先用列表里已有的字段填表（姓名/编号/日期/症状），详情字段先清空
    fillFormFromRow(m_rows.at(m_currentRow));

    // ② 详情缓存里有就直接显示；没有就发第二套请求，回包后由 flush_detail() 填
    requestDetailIfNeeded();
}

void RecordWidget::onSave()
{
    const RecordDetail *d = currentDetail();
    if (!d) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("该条病历的详情还没取到，请稍候再保存。"));
        return;
    }
    if (m_currentRow < 0)
        return;

    RecordDetail edited = *d;    // 以原详情为底，只改表单里的可改字段
    readFormInto(edited);

    if (edited.diagnosis.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("诊断不能为空。"));
        return;
    }

    const RecordRow oldRow = m_rows.at(m_currentRow);
    const bool dateOrSymptomChanged = (edited.visitDate != oldRow.visitDate)
                                      || (edited.mainSymptom != oldRow.mainSymptom);

    m_details.insert(edited.recordId, edited);   // 详情缓存更新

    // 列表里的“就诊日期/主要症状”也来自这条记录，跟着同步一下，免得两边不一致
    // （注意：同步只对本地界面生效，SET_RECORD_REQ 里没有这两个字段，不会上包）
    m_rows[m_currentRow].visitDate   = edited.visitDate;
    m_rows[m_currentRow].mainSymptom = edited.mainSymptom;

    // 上行：和新增病例（appointwidget::sendRecord）**同一个包**，逐字段一致；服务端按 meet_id upsert，
    // 本页 meet_id 固定填 0（不关联预约），所以服务端那边看到的就是一次写入。
    int size = 0;
    const QByteArray pack = makeSavePack(edited, &size);
    qDebug().noquote() << QStringLiteral("[查看病例] 保存修改 → 发送 DOCTOR_SET_RECORD：meet_id=0 doctor_id=%1 patient_id=%2"
                                         " record_id=%3 患者=%4 | 诊断:%5 | 治疗意见:%6 | 包长 %7 字节")
                              .arg(CData::m_id)
                              .arg(edited.patientId)
                              .arg(edited.recordId)
                              .arg(edited.name)
                              .arg(edited.diagnosis)
                              .arg(edited.treatPlan)
                              .arg(size);
    emit to_save_record(pack, size);

    // 改完的日期可能已经跳出当前查询范围（比如改到下个月），那样它就从列表里消失
    const bool stillInRange = matchFilter(m_rows.at(m_currentRow));

    m_loading = true;      // 重刷期间别让 onRowChanged 再判脏/再灌表单
    refreshTable();
    m_loading = false;

    QString tip = QStringLiteral("已提交「%1」的病历修改。\n\n"
                                 "该记录已按「新增病例」那套包（DOCTOR_SET_RECORD）发给服务端，"
                                 "meet_id 固定填 0。").arg(edited.name);
    if (dateOrSymptomChanged)
        tip += QStringLiteral("\n\n注意：就诊日期 / 主要症状不在本次提交的包里（协议里没有这两个字段），"
                              "只更新了本地显示。");
    if (!stillInRange)
        tip += QStringLiteral("\n\n注意：该记录的新就诊日期已不在当前查询范围内，所以列表里暂时看不到它。");
    QMessageBox::information(this, QStringLiteral("已提交"), tip);
}

void RecordWidget::onRevert()
{
    const RecordDetail *d = currentDetail();
    if (!d)
        return;
    fillFormFromDetail(*d);   // 从详情缓存重新灌一次 = 丢弃表单里的改动
    qDebug().noquote() << QStringLiteral("[查看病例] 撤销修改，已还原 record_id=%1").arg(d->recordId);
}

//--------------------------------------------------------------- 回包落地（下行）

void RecordWidget::flush_table()
{
    // 第一套回包：GetMedicalRecordTask 已把结果写进 CData::medical_record_list。
    // m_rows 是“上次查询结果”的界面版，这里整体换成新的（筛选已由服务端做，不要在本地再筛一遍，
    // 否则医生刚改的诊断之类本地状态会跟服务端结果打架）。
    // 先把当前选中记录的 record_id 记下来——重建后下标会变，只能靠 id 找回。
    const int keepId = (m_currentRow >= 0 && m_currentRow < m_rows.size())
                           ? m_rows.at(m_currentRow).recordId
                           : -1;

    m_rows.clear();
    m_rows.reserve(int(CData::medical_record_list.size()));
    for (const MEDICAL_RECORD_INFO &info : CData::medical_record_list) {
        RecordRow r;
        r.recordId    = info.record_id;
        r.patientId   = info.patient_id;
        r.state       = info.state;
        r.visitDate   = info.record_time;
        r.name        = info.patient_name;
        r.mainSymptom = info.main_symptom;
        m_rows.append(r);
    }
    m_serverTotal = CData::medical_record_total;

    // 选中还原：原来那条还在新结果里就仍然选中，否则清空（m_currentRow 是 m_rows 下标，必须重映射）
    int newIdx = -1;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).recordId == keepId) {
            newIdx = i;
            break;
        }
    }
    m_currentRow = newIdx;
    if (newIdx < 0) {
        m_pendingDetailId = -1;
        setDetailEnabled(false);
        m_formTip->clear();
    }

    refreshTable();   // 刷表格 + “共 N 条”

    // refreshTable 里恢复选中时 m_loading 是开着的，onRowChanged 被挡住了，
    // 所以这里补一次：选中的那条详情还没缓存就发第二套请求
    if (m_currentRow >= 0 && !currentDetail())
        requestDetailIfNeeded();

    qDebug().noquote() << QStringLiteral("[查看病例] 列表已刷新：服务端返回 %1 条（总数 %2），选中还原到 record_id=%3")
                              .arg(CData::medical_record_list.size())
                              .arg(CData::medical_record_total)
                              .arg(keepId);
}

void RecordWidget::flush_detail()
{
    // 第二套回包：GetMedicalRecordDetailTask 已把详情按 record_id 存进 CData::medical_record_details。
    // 先确定“这次回来的是哪条”：优先认自己正在等的那条；不是的话，
    // 再从 CData 里挑一条本地还没缓存的（这种只更新缓存，不动界面）。
    int id = -1;
    if (m_pendingDetailId >= 0 && CData::medical_record_details.contains(m_pendingDetailId)) {
        id = m_pendingDetailId;
    } else {
        for (auto it = CData::medical_record_details.constBegin();
             it != CData::medical_record_details.constEnd(); ++it) {
            if (!m_details.contains(it.key())) {
                id = it.key();
                break;
            }
        }
    }
    if (id < 0)
        return;   // 没有新详情（比如同一条又回了一次），不用动界面

    const MEDICAL_RECORD_DETAIL_INFO &info = CData::medical_record_details.value(id);

    // CData 的结构 → 本页的 RecordDetail（只做字段搬运）
    RecordDetail d;
    d.recordId    = info.record_id;
    d.patientId   = info.patient_id;
    d.state       = info.state;
    d.name        = info.patient_name;
    d.sex         = info.patient_sex;
    d.age         = info.patient_age;
    d.visitDate   = info.record_time;
    d.doctor      = info.doctor_name;
    d.mainSymptom = info.main_symptom;
    d.diagnosis   = info.diagnosis;
    d.treatPlan   = info.treat_plan;
    m_details.insert(id, d);   // 进缓存，之后同一行再选中就直接显示

    const bool isCurrent = (m_currentRow >= 0 && m_currentRow < m_rows.size()
                            && m_rows.at(m_currentRow).recordId == id);
    if (!isCurrent) {
        qDebug().noquote() << QStringLiteral("[查看病例] 详情已缓存 record_id=%1（不是当前选中行，界面不动）").arg(id);
        return;
    }

    // 是“我正等的那条”吗？只有它才跟 m_pendingBase 这套快照对得上，
    // 别的来源（服务端主动推、或别的行回包）没有可比对的基准，一律照灌。
    const bool wasPending = (id == m_pendingDetailId);
    if (wasPending)
        m_pendingDetailId = -1;

    // 等回包期间医生可能已经动过表单（详情字段那时是灰的，动的只可能是日期/症状）：
    // 动过就只解锁按钮、保留他敲的内容，别一把盖掉
    if (wasPending && formDiffersFrom(m_pendingBase)) {
        setDetailEnabled(true);
        m_formTip->setText(QStringLiteral("详情已到（保留你的修改）"));
        qDebug().noquote() << QStringLiteral("[查看病例] 详情已到 record_id=%1，但表单已被修改，不覆盖").arg(id);
    } else {
        fillFormFromDetail(d);   // 内部会 setDetailEnabled(true) 并清掉“详情加载中…”
        qDebug().noquote() << QStringLiteral("[查看病例] 详情已灌入表单 record_id=%1 患者=%2").arg(id).arg(d.name);
    }
}
