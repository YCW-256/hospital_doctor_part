#include "smartschedule.h"
#include "smartplan.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLayout>
#include <QLabel>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QMessageBox>
#include <QMap>
#include <QFrame>
#include <cstring>
#include <algorithm>

namespace {
const char *weekdayName[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};
const char *periodName[3]  = {"上午", "下午", "晚上"};

// 列宽常量（医生名列 / 每个星期几列）
const int kColNameW = 120;
const int kColDayW  = 132;

void copyCStr(char *dst, size_t cap, const QString &s)
{
    QByteArray ba = s.toUtf8();
    size_t n = std::min<size_t>(ba.size(), cap - 1);
    if (n > 0)
        memcpy(dst, ba.constData(), n);
    dst[n] = '\0';
}

// 建一个表头/单元格用的普通文本样式
QString plainStyle(const QString &color = QStringLiteral("#333333"),
                   int size = 13, bool bold = false)
{
    return QString("border: none; color: %1; font-size: %2px;%3")
        .arg(color).arg(size).arg(bold ? QStringLiteral(" font-weight: bold;") : QString());
}
}

SmartScheduleDialog::SmartScheduleDialog(const QDate &monday,
                                         const QString &department,
                                         const QVector<QString> &docNames,
                                         const QVector<int> &docIds,
                                         const GUARD_REPIX_T oldInfo[3][7],
                                         QWidget *parent)
    : QDialog(parent)
    , m_monday(monday)
    , m_department(department)
    , m_docNames(docNames)
    , m_docIds(docIds)
    , m_overwriteRadio(nullptr)
    , m_keepRadio(nullptr)
    , m_capSpin(nullptr)
    , m_matrixContent(nullptr)
    , m_matrixGrid(nullptr)
    , m_summaryLabel(nullptr)
    , m_tableHolder(nullptr)
    , m_tableLay(nullptr)
    , m_genBtn(nullptr)
    , m_applyBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    for (int d = 0; d < 7; ++d)
        m_dayDisable[d] = nullptr;
    memset(m_oldInfo, 0, sizeof(m_oldInfo));
    memcpy(m_oldInfo, oldInfo, sizeof(m_oldInfo));
    memset(m_result, 0, sizeof(m_result));

    buildUi();
}

void SmartScheduleDialog::resultPlan(GUARD_REPIX_T out[3][7]) const
{
    memcpy(out, m_result, sizeof(m_result));
}

QString SmartScheduleDialog::dayText(int d) const
{
    QDate day = m_monday.addDays(d);
    return QString("%1\n%2/%3").arg(QString::fromUtf8(weekdayName[d]))
                               .arg(day.month()).arg(day.day());
}

QString SmartScheduleDialog::doctorNameOf(const GUARD_REPIX_T &cell)
{
    return QString::fromUtf8(cell.name).trimmed();
}

int SmartScheduleDialog::existingIndexOf(const GUARD_REPIX_T &cell) const
{
    if (cell.isfree)
        return -1;
    QString name = doctorNameOf(cell);
    if (name.isEmpty())
        return -3;                     // 没名字又非空：当成外人占用，原样保留
    for (int i = 0; i < m_docNames.size(); ++i)
        if (m_docNames[i].trimmed() == name)
            return i;
    return -3;                         // 值班医生不在本科室表里：外人，保留
}

void SmartScheduleDialog::buildUi()
{
    setWindowTitle(QStringLiteral("智能排班"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SmartScheduleDialog { background: #F4FAFF; }");
    resize(1000, 720);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 10);
    root->setSpacing(8);

    // 标题行
    QString dept = m_department.isEmpty() ? QStringLiteral("未选科室") : m_department;
    QLabel *title = new QLabel(QStringLiteral("智能排班 —— 科室：%1 · 排班周：%2 ～ %3")
                                   .arg(dept)
                                   .arg(m_monday.toString(QStringLiteral("yyyy年M月d日")))
                                   .arg(m_monday.addDays(6).toString(QStringLiteral("M月d日"))),
                               this);
    title->setStyleSheet(plainStyle(QStringLiteral("#1E70BF"), 16, true));
    root->addWidget(title);

    // 模式 / 选项行
    QHBoxLayout *optRow = new QHBoxLayout;
    optRow->setSpacing(16);
    m_overwriteRadio = new QRadioButton(QStringLiteral("覆盖当前已排班"), this);
    m_keepRadio = new QRadioButton(QStringLiteral("保留当前排班(只补空格)"), this);
    m_overwriteRadio->setChecked(true);
    QLabel *capTip = new QLabel(QStringLiteral("每人每天最多排"), this);
    m_capSpin = new QSpinBox(this);
    m_capSpin->setRange(1, 3);
    m_capSpin->setValue(1);
    QLabel *capUnit = new QLabel(QStringLiteral("班"), this);
    QLabel *note = new QLabel(QStringLiteral("（默认尽量排满，每时段一人）"), this);
    note->setStyleSheet(plainStyle(QStringLiteral("#999"), 12));
    for (QLabel *l : {capTip, capUnit}) l->setStyleSheet(plainStyle());
    optRow->addWidget(m_overwriteRadio);
    optRow->addWidget(m_keepRadio);
    optRow->addStretch();
    optRow->addWidget(capTip);
    optRow->addWidget(m_capSpin);
    optRow->addWidget(capUnit);
    optRow->addWidget(note);
    root->addLayout(optRow);

    // 禁排矩阵（可叠加）
    QGroupBox *box = new QGroupBox(QStringLiteral("禁排设置（可叠加：整日禁排 / 某医生某天禁排）"), this);
    QVBoxLayout *boxLay = new QVBoxLayout(box);
    boxLay->setContentsMargins(8, 6, 8, 8);

    QLabel *legend = new QLabel(QStringLiteral("勾选 = 该医生该天不可排班；顶部“整日禁排”= 该天所有医生都不排。"), box);
    legend->setStyleSheet(plainStyle(QStringLiteral("#666"), 12));
    boxLay->addWidget(legend);

    QScrollArea *scroll = new QScrollArea(box);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    m_matrixContent = new QWidget;
    m_matrixGrid = new QGridLayout(m_matrixContent);
    m_matrixGrid->setContentsMargins(4, 4, 4, 4);
    m_matrixGrid->setHorizontalSpacing(0);
    m_matrixGrid->setVerticalSpacing(4);
    m_matrixGrid->setColumnMinimumWidth(0, kColNameW);
    for (int d = 0; d < 7; ++d)
        m_matrixGrid->setColumnMinimumWidth(d + 1, kColDayW);
    scroll->setWidget(m_matrixContent);
    boxLay->addWidget(scroll, 1);
    root->addWidget(box, 3);

    // 表头行(第 0 行)：医生列头 + 每列日期 + 整日禁排勾
    QLabel *headName = new QLabel(QStringLiteral("医生"), m_matrixContent);
    headName->setStyleSheet(plainStyle(QStringLiteral("#1E70BF"), 13, true));
    m_matrixGrid->addWidget(headName, 0, 0, Qt::AlignCenter);
    for (int d = 0; d < 7; ++d) {
        QWidget *colBox = new QWidget(m_matrixContent);
        colBox->setFixedWidth(kColDayW);
        QVBoxLayout *cl = new QVBoxLayout(colBox);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(0);
        QLabel *date = new QLabel(dayText(d), colBox);
        date->setAlignment(Qt::AlignCenter);
        date->setStyleSheet(plainStyle(QStringLiteral("#1E70BF"), 13, true));
        m_dayDisable[d] = new QCheckBox(QStringLiteral("整日禁排"), colBox);
        m_dayDisable[d]->setStyleSheet(QStringLiteral("QCheckBox{ color:#C0504D; font-size:12px; }"));
        cl->addWidget(date);
        cl->addWidget(m_dayDisable[d], 0, Qt::AlignHCenter);
        m_matrixGrid->addWidget(colBox, 0, d + 1, Qt::AlignTop | Qt::AlignHCenter);
    }

    // 医生行
    m_docChecks.clear();
    m_docChecks.reserve(m_docNames.size() * 7);
    for (int r = 0; r < m_docNames.size(); ++r) {
        QLabel *nm = new QLabel(m_docNames[r], m_matrixContent);
        nm->setFixedWidth(kColNameW);
        nm->setStyleSheet(plainStyle(QStringLiteral("#333"), 13, true));
        nm->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_matrixGrid->addWidget(nm, r + 1, 0);
        for (int d = 0; d < 7; ++d) {
            QCheckBox *ck = new QCheckBox(m_matrixContent);
            ck->setFixedWidth(kColDayW);
            m_matrixGrid->addWidget(ck, r + 1, d + 1, Qt::AlignHCenter);
            m_docChecks.append(ck);
        }
    }
    if (m_docNames.isEmpty()) {
        QLabel *empty = new QLabel(QStringLiteral("当前科室没有医生，请先返回选择科室加载医生。"), m_matrixContent);
        empty->setStyleSheet(plainStyle(QStringLiteral("#999"), 13));
        m_matrixGrid->addWidget(empty, 1, 0, 1, 8);
    }

    // 预览区
    QGroupBox *prevBox = new QGroupBox(QStringLiteral("排班预览"), this);
    QVBoxLayout *prevLay = new QVBoxLayout(prevBox);
    prevLay->setContentsMargins(8, 6, 8, 8);
    m_summaryLabel = new QLabel(QStringLiteral("点“生成方案”查看本周排班结果。"), prevBox);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setStyleSheet(plainStyle(QStringLiteral("#333"), 12));
    prevLay->addWidget(m_summaryLabel);

    m_tableHolder = new QWidget(prevBox);
    m_tableLay = new QVBoxLayout(m_tableHolder);
    m_tableLay->setContentsMargins(0, 0, 0, 0);
    m_tableLay->setSpacing(0);
    prevLay->addWidget(m_tableHolder, 1);
    root->addWidget(prevBox, 4);

    // 按钮
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_genBtn = new QPushButton(QStringLiteral("生成方案"), this);
    m_applyBtn = new QPushButton(QStringLiteral("应用并保存"), this);
    m_applyBtn->setEnabled(false);
    for (QPushButton *b : {m_cancelBtn, m_genBtn, m_applyBtn})
        b->setMinimumSize(110, 34);
    m_genBtn->setStyleSheet(QStringLiteral("QPushButton{ background:#E6F0FA; color:#1E70BF; border:1px solid #B7D4F2; border-radius:6px; } QPushButton:hover{ background:#D6E6F5; }"));
    m_applyBtn->setStyleSheet(QStringLiteral("QPushButton{ background:#2F80ED; color:#FFFFFF; border:none; border-radius:6px; } QPushButton:hover{ background:#1E70BF; } QPushButton:disabled{ background:#9CC3EC; }"));
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_genBtn);
    btnRow->addWidget(m_applyBtn);
    root->addLayout(btnRow);

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_genBtn, &QPushButton::clicked, this, &SmartScheduleDialog::onGenerate);
    connect(m_applyBtn, &QPushButton::clicked, this, &SmartScheduleDialog::onApply);
}

void SmartScheduleDialog::clearPreview()
{
    if (!m_tableLay)
        return;
    // 每次预览就是“一整块内容控件”，换新即删旧：删 QWidget 会连带删掉它的
    // 子布局和所有格子，避免手动逐层删布局造成的双重释放
    while (QLayoutItem *item = m_tableLay->takeAt(0)) {
        if (QWidget *w = item->widget())
            delete w;
        delete item;
    }
}

void SmartScheduleDialog::onGenerate()
{
    if (m_docNames.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("智能排班"),
                                 QStringLiteral("当前科室没有医生，无法自动排班。"));
        return;
    }
    qDebug()<<"进入2";
    const int nDoc = m_docNames.size();

    SmartPlanIn in;
    in.doctorIds = m_docIds;
    in.maxShiftPerDoctorPerDay = m_capSpin->value();
    in.keepExisting = m_keepRadio->isChecked();

    // 整日禁排 & 医生×星期 禁排
    for (int d = 0; d < 7; ++d)
        in.dayDisabled[d] = m_dayDisable[d] ? m_dayDisable[d]->isChecked() : false;
    in.doctorDayBlocked.resize(nDoc);
    for (int i = 0; i < nDoc; ++i) {
        in.doctorDayBlocked[i].resize(7);
        for (int d = 0; d < 7; ++d) {
            int pos = i * 7 + d;
            bool blocked = (pos < m_docChecks.size()) && m_docChecks[pos]->isChecked();
            in.doctorDayBlocked[i][d] = blocked;
        }
    }

    // 现有排班 -> 医生序号
    for (int k = 0; k < 3; ++k)
        for (int d = 0; d < 7; ++d)
            in.existing[k][d] = existingIndexOf(m_oldInfo[k][d]);

    SmartPlanOut out = runPlan(in, AlgBalancedWeek);
    if (!out.ok) {
        QMessageBox::warning(this, QStringLiteral("智能排班"), out.warn);
        return;
    }
    m_resultWarn = out.warn;

    // 生成 -> 协议结构（DoctorOrder::m_info 同构，date/time 写全便于按差异发包）
    for (int k = 0; k < 3; ++k) {
        for (int d = 0; d < 7; ++d) {
            GUARD_REPIX_T &cell = m_result[k][d];
            memset(&cell, 0, sizeof(cell));
            int idx = out.assign[k][d];
            if (idx == -3) {
                memcpy(&cell, &m_oldInfo[k][d], sizeof(cell));  // 表外占用：原样保留
                continue;
            }
            copyCStr(cell.date, sizeof(cell.date),
                     m_monday.addDays(d).toString(QStringLiteral("yyyy-MM-dd")));
            cell.time = k;
            if (idx >= 0 && idx < nDoc) {
                cell.isfree = false;
                cell.id = (idx < m_docIds.size()) ? m_docIds[idx] : 0;
                copyCStr(cell.name, sizeof(cell.name), m_docNames[idx].trimmed());
                copyCStr(cell.depart, sizeof(cell.depart), m_department);
            }
            else {
                cell.isfree = true;   // 空格/整日禁排：占位，覆盖时清除目标周
            }
        }
    }
    m_generated = true;

    // 汇总文字：可排/未排满 + 各医生班次数
    QMap<int, int> cnt;
    for (int k = 0; k < 3; ++k)
        for (int d = 0; d < 7; ++d)
            if (out.assign[k][d] >= 0)
                ++cnt[out.assign[k][d]];
    int totalShifts = 0;
    for (auto it = cnt.constBegin(); it != cnt.constEnd(); ++it)
        totalShifts += it.value();
    QStringList parts;
    for (int i = 0; i < nDoc; ++i)
        parts << QStringLiteral("%1 %2班").arg(m_docNames[i]).arg(cnt.value(i, 0));
    QString line = QStringLiteral("生成完成，共排 %1 班；未排满 %2 个时段。")
                       .arg(totalShifts)
                       .arg(out.unfilled);
    if (!m_resultWarn.isEmpty())
        line += QStringLiteral("\n%1").arg(m_resultWarn);
    line += QStringLiteral("\n各医生班次：%1").arg(parts.join(QStringLiteral("，")));
    m_summaryLabel->setText(line);

    renderPreview(out);
    m_applyBtn->setEnabled(true);
}

void SmartScheduleDialog::renderPreview(const SmartPlanOut &out)
{
    clearPreview();

    // 整块预览作为一个 QWidget（自己带 QGridLayout），删旧/换新都干净
    QWidget *content = new QWidget(m_tableHolder);
    QGridLayout *g = new QGridLayout(content);
    g->setContentsMargins(0, 0, 0, 0);
    g->setHorizontalSpacing(0);
    g->setVerticalSpacing(2);
    g->setColumnMinimumWidth(0, kColNameW);
    for (int d = 0; d < 7; ++d)
        g->setColumnMinimumWidth(d + 1, kColDayW);

    // 表头：星期 + 日期
    QLabel *headCell = new QLabel(QStringLiteral("时段"), content);
    headCell->setStyleSheet(plainStyle(QStringLiteral("#333"), 12, true));
    headCell->setAlignment(Qt::AlignCenter);
    g->addWidget(headCell, 0, 0);
    for (int d = 0; d < 7; ++d) {
        QLabel *lbl = new QLabel(dayText(d), content);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet(plainStyle(QStringLiteral("#1E70BF"), 12, true));
        lbl->setFixedWidth(kColDayW);
        g->addWidget(lbl, 0, d + 1);
    }
    // 3 时段行
    for (int k = 0; k < 3; ++k) {
        QLabel *pk = new QLabel(QString::fromUtf8(periodName[k]), content);
        pk->setStyleSheet(plainStyle(QStringLiteral("#333"), 12, true));
        pk->setAlignment(Qt::AlignCenter);
        g->addWidget(pk, k + 1, 0);
        for (int d = 0; d < 7; ++d) {
            QLabel *cell = new QLabel(content);
            cell->setFixedWidth(kColDayW);
            cell->setAlignment(Qt::AlignCenter);
            int idx = out.assign[k][d];
            if (m_dayDisable[d] && m_dayDisable[d]->isChecked()) {
                cell->setText(QStringLiteral("整日禁排"));
                cell->setStyleSheet(plainStyle(QStringLiteral("#C0504D"), 12));
            }
            else if (idx >= 0 && idx < m_docNames.size()) {
                cell->setText(m_docNames[idx]);
                cell->setStyleSheet(plainStyle(QStringLiteral("#1E70BF"), 12, true));
            }
            else if (idx == -3) {
                cell->setText(QStringLiteral("原排"));
                cell->setStyleSheet(plainStyle(QStringLiteral("#8A6D3B"), 12));
            }
            else {
                cell->setText(QStringLiteral("空"));
                cell->setStyleSheet(plainStyle(QStringLiteral("#999"), 12));
            }
            g->addWidget(cell, k + 1, d + 1);
        }
    }

    m_tableLay->addWidget(content);
}

void SmartScheduleDialog::onApply()
{
    if (m_docNames.isEmpty())
        return;
    onGenerate();   // 保证按当前约束的最新结果应用
    if (!m_generated)
        return;
    qDebug()<<"进入";
    QString mode = m_keepRadio->isChecked()
                       ? QStringLiteral("保留模式（只补空格）")
                       : QStringLiteral("覆盖模式（整周重排）");
    int ret = QMessageBox::question(
        this, QStringLiteral("应用排班"),
        QStringLiteral("将把当前生成的排班应用到本周（%1）并发服务器，是否继续？").arg(mode),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes)
        accept();
}
