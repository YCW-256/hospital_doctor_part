#include "recordwidget.h"

#include <QAbstractSpinBox>
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

#include "../MyTcp/cdata.h"

namespace {
// 表格列：0 就诊日期 | 1 姓名 | 2 性别 | 3 年龄 | 4 诊断；第 0 列额外用 UserRole 存它在 m_rows 里的下标
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

// 输入框统一样式
const char *kEditQss =
    "QLineEdit, QDateEdit, QComboBox, QSpinBox{"
    " background:#FFFFFF; border:1px solid #B7D4F2; border-radius:6px;"
    " padding:5px 8px; font-size:14px; color:#333333; min-height:22px; }"
    "QLineEdit:focus, QDateEdit:focus, QComboBox:focus, QSpinBox:focus{ border:1px solid #2F80ED; }"
    "QLineEdit:disabled, QDateEdit:disabled, QComboBox:disabled, QSpinBox:disabled{ background:#F2F6FA; color:#AAB4BE; }";

const char *kMultiEditQss =
    "QTextEdit{ background:#FFFFFF; border:1px solid #B7D4F2; border-radius:6px;"
    " padding:6px; font-size:14px; color:#333333; }"
    "QTextEdit:focus{ border:1px solid #2F80ED; }"
    "QTextEdit:disabled{ background:#F2F6FA; color:#AAB4BE; }";

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
} // namespace

RecordWidget::RecordWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentRow(-1)
    , m_loading(false)
    , m_nameEdit(nullptr)
    , m_dateFrom(nullptr)
    , m_dateTo(nullptr)
    , m_queryBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_table(nullptr)
    , m_countLabel(nullptr)
    , m_fDate(nullptr)
    , m_fName(nullptr)
    , m_fSex(nullptr)
    , m_fAge(nullptr)
    , m_fDoctor(nullptr)
    , m_fDiagnosis(nullptr)
    , m_fTreatPlan(nullptr)
    , m_saveBtn(nullptr)
    , m_revertBtn(nullptr)
{
    buildUi();
    seed();
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

    QLabel *sub = new QLabel(QStringLiteral("按姓名 / 就诊日期查询，选中左侧记录后可修改并保存"), this);
    sub->setStyleSheet("background: transparent; border:none; font-size:13px; color:#8A9AB0;");
    root->addWidget(sub);

    buildQueryBar(root);

    // ---- 主体：左侧结果表 + 右侧可编辑详情 ----
    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(14);
    buildTable(this, body);
    buildForm(this, body);
    root->addLayout(body, 1);

    m_countLabel = new QLabel(this);
    m_countLabel->setStyleSheet("background: transparent; border:none; font-size:13px; color:#8A9AB0;");
    root->addWidget(m_countLabel);

    // ---- 信号槽（本次只做本地联动；上行信号 to_query_record/to_save_record 待接服务端）----
    connect(m_queryBtn, &QPushButton::clicked, this, &RecordWidget::onQuery);
    connect(m_resetBtn, &QPushButton::clicked, this, &RecordWidget::onReset);
    connect(m_saveBtn, &QPushButton::clicked, this, &RecordWidget::onSave);
    connect(m_revertBtn, &QPushButton::clicked, this, &RecordWidget::onRevert);
    // 选中行变化（用 currentCellChanged 而不是 itemSelectionChanged：后者在刷新表格时会反复触发）
    connect(m_table, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) { Q_UNUSED(row); onRowChanged(); });
    // 姓名框回车 = 点查询；两个日期框改完也顺手查一次
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &RecordWidget::onQuery);
    connect(m_dateFrom, &QDateEdit::dateChanged, this, [this](const QDate &) { refreshTable(); });
    connect(m_dateTo, &QDateEdit::dateChanged, this, [this](const QDate &) { refreshTable(); });

    setFormEnabled(false);   // 未选中记录时右侧置灰
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

    QLabel *cap = new QLabel(QStringLiteral("查询结果"), card);
    cap->setStyleSheet("background: transparent; border:none; font-size:15px;"
                       " font-weight:bold; color:#1E70BF;");
    lay->addWidget(cap);

    m_table = new QTableWidget(card);
    m_table->setColumnCount(kColCount);
    m_table->setHorizontalHeaderLabels({QStringLiteral("就诊日期"), QStringLiteral("姓名"),
                                        QStringLiteral("性别"), QStringLiteral("年龄"),
                                        QStringLiteral("诊断")});
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

    QLabel *cap = new QLabel(QStringLiteral("病历详情（可修改）"), card);
    cap->setStyleSheet("background: transparent; border:none; font-size:15px;"
                       " font-weight:bold; color:#1E70BF;");
    lay->addWidget(cap);

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

    m_fDate = makeDateEdit(card, QDate::currentDate());
    addField(QStringLiteral("就诊日期"), m_fDate);

    m_fName = new QLineEdit(card);
    m_fName->setStyleSheet(kEditQss);
    addField(QStringLiteral("姓名"), m_fName);

    m_fSex = new QComboBox(card);
    m_fSex->addItems({QStringLiteral("男"), QStringLiteral("女")});
    m_fSex->setStyleSheet(kEditQss);
    addField(QStringLiteral("性别"), m_fSex);

    m_fAge = new QSpinBox(card);
    m_fAge->setRange(0, 150);
    m_fAge->setStyleSheet(kEditQss);
    addField(QStringLiteral("年龄"), m_fAge);

    m_fDoctor = new QLineEdit(card);
    m_fDoctor->setStyleSheet(kEditQss);
    addField(QStringLiteral("接诊医生"), m_fDoctor);

    QLabel *diagCap = makeLabel(card, QStringLiteral("诊断"));
    lay->addWidget(diagCap);
    m_fDiagnosis = new QTextEdit(card);
    m_fDiagnosis->setPlaceholderText(QStringLiteral("请输入诊断……"));
    m_fDiagnosis->setFixedHeight(80);
    m_fDiagnosis->setStyleSheet(kMultiEditQss);
    lay->addWidget(m_fDiagnosis);

    QLabel *treatCap = makeLabel(card, QStringLiteral("治疗意见（处方）"));
    lay->addWidget(treatCap);
    m_fTreatPlan = new QTextEdit(card);
    m_fTreatPlan->setPlaceholderText(QStringLiteral("请输入治疗意见 / 处方……"));
    m_fTreatPlan->setFixedHeight(100);
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

void RecordWidget::seed()
{
    // 【占位数据】与工作统计页同一套约定：CData::is_check==true 时摆几条演示记录，false 时留空。
    // 接入服务端后这里整个删掉，改由 flush_table() 从 CData 的缓存灌数据。
    if (!CData::is_check)
        return;

    const QDate today = QDate::currentDate();
    struct Seed { const char *name; const char *sex; int age; const char *diag; const char *treat; };
    const Seed seeds[] = {
        {"李强", "男", 43, "风寒感冒", "荆防败毒散加减，3 剂，水煎服，每日一剂"},
        {"王芳", "女", 31, "脾胃虚寒", "理中汤加减，5 剂，忌生冷"},
        {"赵敏", "女", 27, "肝郁气滞", "柴胡疏肝散加减，4 剂，调畅情志"},
        {"陈建国", "男", 58, "痰湿咳嗽", "二陈汤加减，5 剂，戒烟酒"},
        {"孙小雨", "女", 19, "风热感冒", "银翘散加减，3 剂，多饮水"},
    };
    const int n = int(sizeof(seeds) / sizeof(seeds[0]));
    for (int i = 0; i < n; ++i) {
        RecordRow r;
        // 日期都落在本月内（保证默认的本月查询能查到）
        const int day = qMax(1, today.day() - i * 2);
        r.visitDate = QDate(today.year(), today.month(), day).toString(QStringLiteral("yyyy-MM-dd"));
        r.name      = QString::fromUtf8(seeds[i].name);
        r.sex       = QString::fromUtf8(seeds[i].sex);
        r.age       = seeds[i].age;
        r.doctor    = QStringLiteral("本人");
        r.diagnosis = QString::fromUtf8(seeds[i].diag);
        r.treatPlan = QString::fromUtf8(seeds[i].treat);
        m_rows.append(r);
    }
}

bool RecordWidget::matchFilter(const RecordRow &r) const
{
    const QString name = m_nameEdit->text().trimmed();
    if (!name.isEmpty() && !r.name.contains(name, Qt::CaseInsensitive))
        return false;

    // 就诊日期只在“起 ~ 止”之间（字符串比较对 yyyy-MM-dd 成立）
    const QString from = m_dateFrom->date().toString(QStringLiteral("yyyy-MM-dd"));
    const QString to   = m_dateTo->date().toString(QStringLiteral("yyyy-MM-dd"));
    return r.visitDate >= from && r.visitDate <= to;
}

void RecordWidget::refreshTable()
{
    // 重刷表格会触发 currentCellChanged，用 m_loading 挡住 onRowChanged 里的脏数据判断
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
        const QStringList texts = {r.visitDate, r.name, r.sex, QString::number(r.age), r.diagnosis};
        for (int c = 0; c < kColCount; ++c) {
            QTableWidgetItem *item = new QTableWidgetItem(texts.at(c));
            if (c == 0)
                item->setData(kRowIndexRole, i);   // 记住它在 m_rows 里的下标（表格行 ≠ 数据下标）
            m_table->setItem(row, c, item);
        }
        ++hit;
    }
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
        if (!formDiffersFromRow())
            loadRowToForm(m_currentRow);
    } else {
        m_currentRow = -1;
        setFormEnabled(false);
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

void RecordWidget::loadRowToForm(int idx)
{
    if (idx < 0 || idx >= m_rows.size()) {
        setFormEnabled(false);
        return;
    }
    const RecordRow &r = m_rows.at(idx);

    const bool prevLoading = m_loading;
    m_loading = true;   // 灌数据期间不判脏

    m_fDate->setDate(QDate::fromString(r.visitDate, QStringLiteral("yyyy-MM-dd")));
    m_fName->setText(r.name);
    const int sexIdx = m_fSex->findText(r.sex);
    m_fSex->setCurrentIndex(sexIdx >= 0 ? sexIdx : 0);
    m_fAge->setValue(r.age);
    m_fDoctor->setText(r.doctor);
    m_fDiagnosis->setPlainText(r.diagnosis);
    m_fTreatPlan->setPlainText(r.treatPlan);

    m_loading = prevLoading;

    setFormEnabled(true);
}

void RecordWidget::setFormEnabled(bool on)
{
    m_fDate->setEnabled(on);
    m_fName->setEnabled(on);
    m_fSex->setEnabled(on);
    m_fAge->setEnabled(on);
    m_fDoctor->setEnabled(on);
    m_fDiagnosis->setEnabled(on);
    m_fTreatPlan->setEnabled(on);
    m_saveBtn->setEnabled(on);
    m_revertBtn->setEnabled(on);
}

void RecordWidget::readFormInto(RecordRow &r) const
{
    r.visitDate = m_fDate->date().toString(QStringLiteral("yyyy-MM-dd"));
    r.name      = m_fName->text().trimmed();
    r.sex       = m_fSex->currentText();
    r.age       = m_fAge->value();
    r.doctor    = m_fDoctor->text().trimmed();
    r.diagnosis = m_fDiagnosis->toPlainText().trimmed();
    r.treatPlan = m_fTreatPlan->toPlainText().trimmed();
}

bool RecordWidget::formDirty() const
{
    return !m_loading && formDiffersFromRow();
}

bool RecordWidget::formDiffersFromRow() const
{
    if (m_currentRow < 0 || m_currentRow >= m_rows.size())
        return false;
    RecordRow cur;
    readFormInto(cur);
    const RecordRow &old = m_rows.at(m_currentRow);
    return cur.visitDate != old.visitDate || cur.name != old.name || cur.sex != old.sex
           || cur.age != old.age || cur.doctor != old.doctor
           || cur.diagnosis != old.diagnosis || cur.treatPlan != old.treatPlan;
}

void RecordWidget::onQuery()
{
    qDebug().noquote() << QStringLiteral("[查看病例] 查询：姓名=%1  日期 %2 ~ %3")
                              .arg(m_nameEdit->text().trimmed().isEmpty() ? QStringLiteral("(全部)")
                                                                         : m_nameEdit->text().trimmed())
                              .arg(m_dateFrom->date().toString(QStringLiteral("yyyy-MM-dd")))
                              .arg(m_dateTo->date().toString(QStringLiteral("yyyy-MM-dd")));
    // TODO(数据通信)：这里将来 emit to_query_record()，由窗口打包后走 SocketLink::send_data
    emit to_query_record();

    refreshTable();
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
    refreshTable();
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
    loadRowToForm(m_currentRow);
}

void RecordWidget::onSave()
{
    if (m_currentRow < 0) {
        QMessageBox::information(this, QStringLiteral("提示"),
                                 QStringLiteral("请先在左侧选中一条记录。"));
        return;
    }

    // 先把当前表单读进结构体（表格里的下标要趁改之前取，因为日期可能被改到查询范围之外）
    RecordRow edited;
    readFormInto(edited);

    if (edited.name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("姓名不能为空。"));
        return;
    }

    const int idx = m_currentRow;
    const RecordRow before = m_rows.at(idx);

    m_rows[idx] = edited;
    qDebug().noquote() << QStringLiteral("[查看病例] 保存修改：%1 → %2")
                              .arg(before.name + QStringLiteral("@") + before.visitDate)
                              .arg(edited.name + QStringLiteral("@") + edited.visitDate)
                       << QStringLiteral("| 诊断:") << edited.diagnosis
                       << QStringLiteral("| 治疗意见:") << edited.treatPlan;

    // 预留给数据通信的上行信号：现在没有接收方，只是先把“按钮 → 信号”这半截接通；
    // 接入服务端时改成带打包数据（QByteArray）再连到 SocketLink::send_data。
    emit to_save_record();

    // 改完的就诊日期可能已经跳出当前查询范围（比如把日期改到下个月），那样它就从列表里消失
    const bool stillInRange = matchFilter(edited);

    m_loading = true;      // 重刷期间别让 onRowChanged 再判脏
    refreshTable();
    m_loading = false;

    QString tip = QStringLiteral("已保存「%1」的病历修改（本地演示数据，尚未提交服务端）。").arg(edited.name);
    if (!stillInRange)
        tip += QStringLiteral("\n\n注意：该记录的新就诊日期已不在当前查询范围内，所以列表里暂时看不到它。");
    QMessageBox::information(this, QStringLiteral("保存成功"), tip);
}

void RecordWidget::onRevert()
{
    if (m_currentRow < 0)
        return;
    loadRowToForm(m_currentRow);   // 从数据行重新灌一次 = 丢弃表单里的改动
    qDebug().noquote() << QStringLiteral("[查看病例] 撤销修改，已还原为原记录");
}

void RecordWidget::flush_table()
{
    // 预留：服务端回包后由 MainWindow 调用（现在只是按本地数据重刷）
    refreshTable();
}
