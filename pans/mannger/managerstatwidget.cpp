#include "managerstatwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRandomGenerator>
#include <QDebug>
#include "../../Tool/myutils.h"
#include "../../MyTcp/cdata.h"

// ============================ TrendChartWidget（自绘折线） ============================

TrendChartWidget::TrendChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(280);
}

void TrendChartWidget::setSeries(const QStringList &xLabels, const QVector<int> &values)
{
    m_labels = xLabels;
    m_values = values;
    update();
}

void TrendChartWidget::clearSeries()
{
    m_labels.clear();
    m_values.clear();
    update();
}

void TrendChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int W = width();
    const int H = height();

    // 白底圆角卡片
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#FFFFFF"));
    p.drawRoundedRect(QRectF(0.5, 0.5, W - 1, H - 1), 10, 10);

    const int mL = 56, mR = 18, mT = 18, mB = 30;
    const int plotW = W - mL - mR;
    const int plotH = H - mT - mB;
    if (plotW <= 10 || plotH <= 10)
        return;

    if (m_values.isEmpty() || m_labels.isEmpty()) {
        QFont f = p.font();
        f.setPointSize(13);
        p.setFont(f);
        p.setPen(QColor("#9AA5B1"));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("暂无数据"));
        return;
    }

    // 顶值取最大值再放一点余量
    int maxV = 0;
    for (int v : m_values)
        maxV = qMax(maxV, v);
    if (maxV <= 0)
        maxV = 1;
    const double top = maxV * 1.15;

    QFont small = p.font();
    small.setPointSize(8);
    p.setFont(small);

    // 横向网格 + 左侧刻度值
    for (int g = 0; g <= 4; ++g) {
        const double fr = g / 4.0;
        const int y = mT + int(plotH * (1.0 - fr));
        p.setPen(QPen(QColor("#DEEBF7"), 1));
        p.drawLine(mL, y, mL + plotW, y);
        p.setPen(QColor("#8A9AB0"));
        p.drawText(QRect(0, y - 7, mL - 6, 14), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(int(top * fr + 0.5)));
    }

    const int n = m_values.size();
    auto xAt = [&](int i) -> double {
        return mL + (n <= 1 ? plotW / 2.0 : plotW * double(i) / double(n - 1));
    };
    auto yAt = [&](int v) -> double {
        return mT + plotH * (1.0 - v / top);
    };

    // 渐变面积
    QPainterPath area;
    area.moveTo(xAt(0), mT + plotH);
    for (int i = 0; i < n; ++i)
        area.lineTo(xAt(i), yAt(m_values[i]));
    area.lineTo(xAt(n - 1), mT + plotH);
    area.closeSubpath();
    QLinearGradient grad(0, mT, 0, mT + plotH);
    grad.setColorAt(0.0, QColor(0x2F, 0x80, 0xED, 90));
    grad.setColorAt(1.0, QColor(0x2F, 0x80, 0xED, 8));
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPath(area);

    // 折线
    QPainterPath line;
    line.moveTo(xAt(0), yAt(m_values[0]));
    for (int i = 1; i < n; ++i)
        line.lineTo(xAt(i), yAt(m_values[i]));
    p.setPen(QPen(QColor("#2F80ED"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(line);

    // 数据点
    for (int i = 0; i < n; ++i) {
        p.setPen(QPen(QColor("#2F80ED"), 1.5));
        p.setBrush(QColor("#FFFFFF"));
        p.drawEllipse(QPointF(xAt(i), yAt(m_values[i])), 3.0, 3.0);
    }

    // 底部横轴标签（点数太多时跳着画，避免重叠）
    const int step = n > 12 ? (n + 11) / 12 : 1;
    p.setPen(QColor("#5A6B7F"));
    for (int i = 0; i < n; i += step) {
        p.drawText(QRect(int(xAt(i)) - 32, mT + plotH + 6, 64, 18),
                   Qt::AlignHCenter | Qt::AlignTop, m_labels.value(i));
    }
}

// ============================ ManagerStatWidget ============================

ManagerStatWidget::ManagerStatWidget(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_toggleBtn(nullptr)
    , m_stack(nullptr)
    , m_chartPage(nullptr)
    , m_workloadPage(nullptr)
    , m_chart(nullptr)
    , m_workTable(nullptr)
{
    buildUi();
    // 预置占位数据（is_check 决定是否填充）
    seedTrendByScale();
    refreshTrend();
    seedWorkload();
}

void ManagerStatWidget::buildUi()
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("ManagerStatWidget { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                  " stop:0 #E8F2F6, stop:1 #FFFFFF); }");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 16);
    root->setSpacing(12);

    // 顶栏：标题 + 切换按钮
    QHBoxLayout *head = new QHBoxLayout;
    m_titleLabel = new QLabel(QStringLiteral("工作统计"), this);
    m_titleLabel->setStyleSheet("border:none; font-size:19px; font-weight:bold; color:#1E70BF;");
    m_toggleBtn = new QPushButton(QStringLiteral("医生工作量"), this);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBtn->setMinimumSize(104, 34);
    m_toggleBtn->setStyleSheet(
        "QPushButton{ background:#2F80ED; color:#FFFFFF; border:none; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#1E70BF; }");
    head->addWidget(m_titleLabel);
    head->addStretch();
    head->addWidget(m_toggleBtn);
    root->addLayout(head);

    // 内容：两个子视图
    m_stack = new QStackedWidget(this);

    // ---- 视图1：访问量趋势（折线图 + 日/周/月/年 分段按钮） ----
    m_chartPage = new QWidget(this);
    m_chartPage->setAttribute(Qt::WA_StyledBackground, true);
    QVBoxLayout *chartLay = new QVBoxLayout(m_chartPage);
    chartLay->setContentsMargins(0, 0, 0, 0);
    chartLay->setSpacing(10);

    QHBoxLayout *ctrl = new QHBoxLayout;
    QLabel *tip = new QLabel(QStringLiteral("门诊访问量趋势"), m_chartPage);
    tip->setStyleSheet("border:none; font-size:16px; font-weight:bold; color:#333333;");
    ctrl->addWidget(tip);
    ctrl->addStretch();

    // 日/周/月/年 分段按钮组（样式与 childs/selbtn 类似，但 4 档）
    QButtonGroup *scaleGroup = new QButtonGroup(m_chartPage);
    scaleGroup->setExclusive(true);
    const QStringList scaleNames = {QStringLiteral("日"), QStringLiteral("周"),
                                    QStringLiteral("月"), QStringLiteral("年")};
    for (int i = 0; i < scaleNames.size(); ++i) {
        QPushButton *btn = new QPushButton(scaleNames.at(i), m_chartPage);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton{ background:#FFFFFF; border:1px solid #B7D4F2; border-radius:6px;"
            " padding:6px 18px; font-size:14px; color:#555555; }"
            "QPushButton:hover{ color:#1E70BF; }"
            "QPushButton:checked{ background:#2F80ED; color:#FFFFFF; border-color:#2F80ED; font-weight:bold; }");
        scaleGroup->addButton(btn, i);
        ctrl->addWidget(btn);
    }
    scaleGroup->button(0)->setChecked(true);
    chartLay->addLayout(ctrl);

    m_chart = new TrendChartWidget(m_chartPage);
    chartLay->addWidget(m_chart, 1);
    m_stack->addWidget(m_chartPage);

    // ---- 视图2：医生工作量表 ----
    m_workloadPage = new QWidget(this);
    m_workloadPage->setAttribute(Qt::WA_StyledBackground, true);
    QVBoxLayout *workLay = new QVBoxLayout(m_workloadPage);
    workLay->setContentsMargins(0, 0, 0, 0);
    workLay->setSpacing(10);

    QLabel *workTip = new QLabel(QStringLiteral("每位医生工作量（本月）"), m_workloadPage);
    workTip->setStyleSheet("border:none; font-size:16px; font-weight:bold; color:#333333;");
    workLay->addWidget(workTip);

    m_workTable = new QTableWidget(m_workloadPage);
    m_workTable->setColumnCount(4);
    m_workTable->setHorizontalHeaderLabels(
        {QStringLiteral("医生"), QStringLiteral("月出勤天数"),
         QStringLiteral("月访问量"), QStringLiteral("月加班次数")});
    m_workTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_workTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_workTable->setShowGrid(true);
    m_workTable->verticalHeader()->setVisible(false);
    m_workTable->setStyleSheet(
        "QTableWidget{ background:#FFFFFF; border:1px solid #D9E7F5; gridline-color:#E6EFF8; font-size:14px; }"
        "QHeaderView::section{ background:#EAF3FB; color:#1E70BF; font-weight:bold; font-size:14px;"
        " border:none; border-bottom:1px solid #D9E7F5; padding:8px; }"
        "QTableWidget::item{ padding:2px 6px; }");
    m_workTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_workTable->horizontalHeader()->setMinimumHeight(36);
    m_workTable->verticalHeader()->setDefaultSectionSize(38);
    workLay->addWidget(m_workTable, 1);
    m_stack->addWidget(m_workloadPage);

    root->addWidget(m_stack, 1);

    connect(scaleGroup, &QButtonGroup::idClicked, this, &ManagerStatWidget::onScaleChanged);
    connect(m_toggleBtn, &QPushButton::clicked, this, &ManagerStatWidget::onToggleView);
}

void ManagerStatWidget::seedTrendByScale()
{
    m_xLabels.clear();
    m_values.clear();
    if (!CData::is_check)
        return; // 非预置模式：留空，等真实统计

    QRandomGenerator *r = QRandomGenerator::global();
    switch (m_scale) {
    case 0: { // 日：按 8 个时段
        const QStringList names = {QStringLiteral("00-03"), QStringLiteral("03-06"),
                                   QStringLiteral("06-09"), QStringLiteral("09-12"),
                                   QStringLiteral("12-15"), QStringLiteral("15-18"),
                                   QStringLiteral("18-21"), QStringLiteral("21-24")};
        m_xLabels = names;
        for (int i = 0; i < names.size(); ++i)
            m_values.append(r->bounded(5, 90));
        break;
    }
    case 1: { // 周：周一~周日
        const QStringList names = {QStringLiteral("周一"), QStringLiteral("周二"),
                                   QStringLiteral("周三"), QStringLiteral("周四"),
                                   QStringLiteral("周五"), QStringLiteral("周六"),
                                   QStringLiteral("周日")};
        m_xLabels = names;
        for (int i = 0; i < names.size(); ++i)
            m_values.append(r->bounded(60, 300));
        break;
    }
    case 2: { // 月：1~30 号
        for (int d = 1; d <= 30; ++d) {
            m_xLabels.append(QString::number(d));
            m_values.append(r->bounded(80, 300));
        }
        break;
    }
    case 3: { // 年：1~12 月
        for (int m = 1; m <= 12; ++m) {
            m_xLabels.append(QStringLiteral("%1月").arg(m));
            m_values.append(r->bounded(1600, 3600));
        }
        break;
    }
    default:
        break;
    }
}

void ManagerStatWidget::refreshTrend()
{
    m_chart->setSeries(m_xLabels, m_values);
}

void ManagerStatWidget::seedWorkload()
{
    m_workTable->setRowCount(0);
    if (!CData::is_check)
        return; // 非预置模式：留空，等真实统计

    const QStringList names = {QStringLiteral("张医生"), QStringLiteral("李医生"),
                               QStringLiteral("王医生"), QStringLiteral("刘医生"),
                               QStringLiteral("陈医生"), QStringLiteral("杨医生"),
                               QStringLiteral("赵医生"), QStringLiteral("黄医生")};
    QRandomGenerator *r = QRandomGenerator::global();
    m_workTable->setRowCount(names.size());
    for (int i = 0; i < names.size(); ++i) {
        QTableWidgetItem *nameItem = new QTableWidgetItem(names.at(i));
        nameItem->setTextAlignment(Qt::AlignCenter);
        m_workTable->setItem(i, 0, nameItem);

        auto makeNum = [&](int lo, int hi) {
            QTableWidgetItem *it = new QTableWidgetItem(QString::number(r->bounded(lo, hi)));
            it->setTextAlignment(Qt::AlignCenter);
            return it;
        };
        m_workTable->setItem(i, 1, makeNum(18, 27)); // 月出勤天数
        m_workTable->setItem(i, 2, makeNum(80, 330));// 月访问量
        m_workTable->setItem(i, 3, makeNum(0, 11));  // 月加班次数
    }
}

void ManagerStatWidget::onScaleChanged(int idx)
{
    if (idx < 0 || idx > 3)
        return;
    m_scale = idx;
    seedTrendByScale();
    refreshTrend();
}

void ManagerStatWidget::onToggleView()
{
    // 默认在趋势图视图；切到医生工作量，或切回来
    const bool goWork = (m_stack->currentWidget() == m_chartPage);
    if (goWork) {
        m_stack->setCurrentWidget(m_workloadPage);
        m_toggleBtn->setText(QStringLiteral("查看趋势"));
    } else {
        m_stack->setCurrentWidget(m_chartPage);
        m_toggleBtn->setText(QStringLiteral("医生工作量"));
    }
}
