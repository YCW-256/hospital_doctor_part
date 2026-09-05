#include "doctororder.h"
#include "childs/mannger/doctorcard.h"
#include "childs/mannger/copyschedule.h"
#include "../MyTcp/cdata.h"
#include "../MyTcp/protecol.h"
#include <QComboBox>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStringList>
#include <QFrame>
#include <QLayoutItem>
#include <QSpacerItem>
#include <QDebug>
#include <string.h>
#include <algorithm>
#include <cstddef>

static const QString periodNames[3] = {"上午", "下午", "晚上"};
static const char *weekName[7] = {"周一", "周二", "周三", "周四", "周五", "周六", "周日"};

// 把 QString 拷贝进定长 char 数组（UTF-8 字节），带结束符
static void copyCStr(char *dst, size_t cap, const QString &s)
{
    QByteArray ba = s.toUtf8();
    size_t n = std::min<size_t>(ba.size(), cap - 1);
    if (n > 0)
        memcpy(dst, ba.constData(), n);
    dst[n] = '\0';
}

// 排班表尺寸约定：左列医生名 / 顶部时间栏 / 表格三者靠这些常量对齐
static const int kNameW    = 110;   // 医生名列宽
static const int kCardW    = 92;    // 单张时段卡宽
static const int kRowH     = 40;    // 行高（与 DoctorSlotCard 高一致）
static const int kGap      = 8;     // 天与天(及行与行)之间的间距
static const int kTimeBarH = 52;    // 顶部时间栏高
static const int kDayW     = 3 * kCardW; // 一天块宽 = 三卡无间距

DoctorOrder::DoctorOrder(QWidget *parent)
    : QWidget(parent)
    , m_deptCombo(nullptr)
    , m_prevBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_copyBtn(nullptr)
    , m_nextBtn(nullptr)
    , m_timeBar(nullptr)
    , m_scroll(nullptr)
    , m_nameScroll(nullptr)
    , m_nameLayout(nullptr)
    , m_copyTimer(nullptr)
{
    QDate today = QDate::currentDate();
    m_monday = today.addDays(-(today.dayOfWeek() - 1)); // 默认从今天所在周的周一开始

    // 批量复制逐周发送的节奏定时器（代替连发包之间的 sleep，界面不卡）
    m_copyTimer = new QTimer(this);
    m_copyTimer->setInterval(250);
    connect(m_copyTimer, &QTimer::timeout, this, &DoctorOrder::sendNextCopyPack);

    initInfo();
    buildShell();
    refreshDates();     // 时间栏先按当前周显示，与是否查询医生无关
    rebuildContent();
}

DoctorOrder::~DoctorOrder()
{
}

QString DoctorOrder::dateText(const QDate &monday, int offset)
{
    QDate day = monday.addDays(offset);
    return QString("%1\n%2月%3日").arg(QString::fromUtf8(weekName[offset]))
                                  .arg(day.month()).arg(day.day());
}

void DoctorOrder::buildShell()
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---------- 顶部工具条：上一周 | 选择科室 | 保存修改 | ... | 下一周 ----------
    QWidget *bar = new QWidget(this);
    QHBoxLayout *bl = new QHBoxLayout(bar);
    bl->setContentsMargins(16, 10, 16, 6);
    bl->setSpacing(10);

    m_prevBtn = new QPushButton("上一周", bar);
    m_deptCombo = new QComboBox(bar);
    m_deptCombo->addItem("科室");    // 占位项(index 0)，不触发查询
    m_deptCombo->addItem("内科");
    m_deptCombo->addItem("外科");
    m_saveBtn = new QPushButton("保存修改", bar);
    m_copyBtn = new QPushButton("批量复制排班", bar);
    m_nextBtn = new QPushButton("下一周", bar);

    for (QPushButton *btn : {m_prevBtn, m_saveBtn, m_copyBtn, m_nextBtn})
        btn->setMinimumHeight(30);
    m_deptCombo->setMinimumHeight(30);

    bl->addWidget(m_prevBtn);
    bl->addWidget(m_deptCombo);
    bl->addWidget(m_saveBtn);
    bl->addWidget(m_copyBtn);
    bl->addStretch();            // 上一周靠左、下一周靠右
    bl->addWidget(m_nextBtn);
    root->addWidget(bar);

    // ================= 中部：左列固定医生名 + 右侧(时间栏/表格) =================
    QWidget *center = new QWidget(this);
    QHBoxLayout *cl = new QHBoxLayout(center);
    cl->setContentsMargins(0, 0, 0, 0);
    cl->setSpacing(0);

    // ---- 左侧固定列：顶部空角 + 医生名纵向列表 ----
    QWidget *namePanel = new QWidget(center);
    namePanel->setFixedWidth(kNameW);
    QVBoxLayout *np = new QVBoxLayout(namePanel);
    np->setContentsMargins(0, 0, 0, 0);
    np->setSpacing(0);

    QWidget *corner = new QWidget(namePanel);   // 与右侧时间栏同高的空角
    corner->setFixedHeight(kTimeBarH);
    np->addWidget(corner);

    m_nameScroll = new QScrollArea(namePanel);
    m_nameScroll->setWidgetResizable(true);
    m_nameScroll->setFrameShape(QFrame::NoFrame);
    m_nameScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nameScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *nameContent = new QWidget;
    m_nameLayout = new QVBoxLayout(nameContent);
    // 顶部/底部外边距与表格内容一致，保证行对齐
    m_nameLayout->setContentsMargins(0, 8, 0, 16);
    m_nameLayout->setSpacing(kGap);
    m_nameScroll->setWidget(nameContent);
    np->addWidget(m_nameScroll, 1);

    cl->addWidget(namePanel);

    // ---- 右侧：顶部时间栏 + 下方可滚动表格 ----
    QWidget *right = new QWidget(center);
    QVBoxLayout *rl = new QVBoxLayout(right);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    m_timeBar = new QScrollArea(right);
    m_timeBar->setWidgetResizable(true);
    m_timeBar->setFrameShape(QFrame::NoFrame);
    m_timeBar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_timeBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 横向只保留最下方表格那条
    m_timeBar->setFixedHeight(kTimeBarH);

    QWidget *timeContent = new QWidget(m_timeBar);
    QHBoxLayout *tl = new QHBoxLayout(timeContent);
    tl->setContentsMargins(16, 6, 16, 4);
    tl->setSpacing(kGap);

    m_dateLabels.clear();
    for (int d = 0; d < 7; ++d) {
        QLabel *date = new QLabel(QString(), timeContent);
        date->setAlignment(Qt::AlignCenter);
        date->setFixedWidth(kDayW);
        date->setStyleSheet("border: none; font-size: 13px; font-weight: bold; color: #1E70BF;");
        tl->addWidget(date);
        m_dateLabels.push_back(date);
    }

    // 时间栏内容宽 = 表格内容宽（无医生名列），与表格横向范围一致
    int timeW = 32 + 7 * kDayW + 6 * kGap;
    timeContent->setMinimumWidth(timeW);
    m_timeBar->setWidget(timeContent);
    rl->addWidget(m_timeBar);

    m_scroll = new QScrollArea(right);
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    rl->addWidget(m_scroll, 1);

    cl->addWidget(right, 1);
    root->addWidget(center, 1);

    // ---- 滚动联动：时间栏横向随表格、医生名列纵向随表格 ----
    connect(m_scroll->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_timeBar->horizontalScrollBar(), &QScrollBar::setValue);
    connect(m_timeBar->horizontalScrollBar(), &QScrollBar::valueChanged,
            m_scroll->horizontalScrollBar(), &QScrollBar::setValue);

    connect(m_scroll->verticalScrollBar(), &QScrollBar::valueChanged,
            m_nameScroll->verticalScrollBar(), &QScrollBar::setValue);
    connect(m_nameScroll->verticalScrollBar(), &QScrollBar::valueChanged,
            m_scroll->verticalScrollBar(), &QScrollBar::setValue);

    // ---- 工具条连接 ----
    connect(m_prevBtn, &QPushButton::clicked, this, &DoctorOrder::prevWeek);
    connect(m_nextBtn, &QPushButton::clicked, this, &DoctorOrder::nextWeek);
    connect(m_saveBtn, &QPushButton::clicked, this, &DoctorOrder::saveInfo);
    connect(m_copyBtn, &QPushButton::clicked, this, &DoctorOrder::openCopyDialog);
    connect(m_deptCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DoctorOrder::onDeptChanged);
}

void DoctorOrder::rebuildContent()
{
    // 清空医生名列（人名 label + 底部弹簧）
    while (QLayoutItem *item = m_nameLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            delete w;
        delete item;
    }
    m_nameLabels.clear();

    // 清空右侧表格
    if (QWidget *old = m_scroll->takeWidget())
        delete old;
    m_cards.clear();

    QWidget *content = new QWidget(m_scroll);
    QGridLayout *grid = new QGridLayout(content);
    // 外边距/行距与医生名列(左侧)保持一致，保证行高对齐
    grid->setContentsMargins(16, 8, 16, 16);
    grid->setHorizontalSpacing(kGap);
    grid->setVerticalSpacing(kGap);

    int row = 0;
    if (m_docNames.isEmpty()) {
        QLabel *hint = new QLabel("请选择科室加载医生，开始编排排班", content);
        hint->setAlignment(Qt::AlignCenter);
        hint->setMinimumHeight(90);
        hint->setStyleSheet("border: none; color: #999; font-size: 14px;");
        grid->addWidget(hint, row, 0, 1, 7);
    }
    else {
        for (int r = 0; r < m_docNames.size(); ++r, ++row) {
            // 左列：医生名，高度 kRowH 与卡片行一致
            QLabel *name = new QLabel(m_docNames[r]);
            name->setFixedSize(kNameW, kRowH);
            name->setAlignment(Qt::AlignCenter);
            name->setStyleSheet("border: none; font-size: 14px; font-weight: bold; color: #333;");
            m_nameLayout->addWidget(name);
            m_nameLabels.push_back(name);

            // 右侧表格一行：7 天，每天 = 三个时段卡无间距拼接
            for (int d = 0; d < 7; ++d) {
                QWidget *dayGroup = new QWidget(content);
                QHBoxLayout *hl = new QHBoxLayout(dayGroup);
                hl->setContentsMargins(0, 0, 0, 0);
                hl->setSpacing(0);
                for (int k = 0; k < 3; ++k) {
                    DoctorSlotCard *card = new DoctorSlotCard(periodNames[k], dayGroup);
                    hl->addWidget(card);
                    m_cards.push_back(card);
                    // 点击卡片：指派/取消该行医生在此(天,时段)的值班（参照 orderWidget）
                    connect(card, &DoctorSlotCard::clicked, this,
                            [this, r, d, k]() { onCardToggled(r, d, k); });
                }
                grid->addWidget(dayGroup, row, d);
            }
        }
    }

    // 医生名列底部弹簧：剩余高度吃在下方，名字保持顶部对齐
    m_nameLayout->addStretch();

    // 底部弹簧吃掉剩余高度，让排班行在顶部紧凑排布
    grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
                  row, 0, 1, 7);

    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_scroll->setWidget(content);
}

void DoctorOrder::refreshDates()
{
    for (int i = 0; i < m_dateLabels.size() && i < 7; ++i)
        m_dateLabels[i]->setText(dateText(m_monday, i));
}

void DoctorOrder::prevWeek()
{
    m_monday = m_monday.addDays(-7);
    refreshDates();
    qDebug() << "切到上一周，周一:" << m_monday.toString("yyyy-MM-dd");
    if (m_deptCombo->currentIndex() > 0) { // 已选科室则重查该周排班
        initInfo();
        applyRoster();
        sendGuardRequest();
    }
}

void DoctorOrder::nextWeek()
{
    m_monday = m_monday.addDays(7);
    refreshDates();
    qDebug() << "切到下一周，周一:" << m_monday.toString("yyyy-MM-dd");
    if (m_deptCombo->currentIndex() > 0) {
        initInfo();
        applyRoster();
        sendGuardRequest();
    }
}

void DoctorOrder::onDeptChanged(int index)
{
    if (index <= 0)
        return; // 占位项"科室"不查询

    QString dept = m_deptCombo->currentText();
    if (dept.isEmpty())
        return;

    HEAD head;
    memset(&head, 0, sizeof(head));
    head.type = SERVICE_TYPE::SELECT_DOCTOR;
    head.is_fragment = false;
    head.len = sizeof(DOCTOR_INFO_REQ);

    DOCTOR_INFO_REQ req;
    memset(&req, 0, sizeof(req));
    QByteArray ba = dept.toUtf8();
    size_t n = std::min<size_t>(ba.size(), sizeof(req.department) - 1);
    memcpy(req.department, ba.constData(), n);
    req.department[n] = '\0';
    req.id = CData::m_id;

    int send_size = sizeof(HEAD) + sizeof(req);
    QByteArray data;
    data.resize(send_size);
    memcpy(data.data(), &head, sizeof(HEAD));
    memcpy(data.data() + sizeof(HEAD), &req, sizeof(req));

    qDebug() << "DoctorOrder 发送科室医生查询:" << dept << "id" << req.id;
    emit get_doctor_info(data, send_size);

    // 换了科室：清掉旧排班，随后查新科室本周排班（sendGuardRequest 内先 sleep，目前这样）
    initInfo();
    sendGuardRequest();
}

void DoctorOrder::initInfo()
{
    memset(m_info, 0, sizeof(m_info));
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 7; ++j)
            m_info[i][j].isfree = true; // 默认空闲
    // 清空每位医生的 21 位修改标记（换科室/换周时重来）
    for (DocModified &row : m_modified)
        memset(row.flag, 0, sizeof(row.flag));
}

void DoctorOrder::sendGuardRequest()
{
    if (m_deptCombo->currentIndex() <= 0)
        return;
    QString dept = m_deptCombo->currentText();
    if (dept.isEmpty())
        return;

    QThread::msleep(100); // 目前这样：缓解 SELECT/GET_GUARD 响应并发两包

    HEAD head;
    memset(&head, 0, sizeof(head));
    head.type = SERVICE_TYPE::GET_GUARD;
    head.is_fragment = false;
    head.len = sizeof(GET_GUARD_REQ);

    GET_GUARD_REQ req;
    memset(&req, 0, sizeof(req));
    copyCStr(req.department, sizeof(req.department), dept);
    copyCStr(req.start_day, sizeof(req.start_day), m_monday.toString("yyyy-MM-dd"));
    req.id = CData::m_id;

    int send_size = sizeof(HEAD) + sizeof(req);
    QByteArray data;
    data.resize(send_size);
    memcpy(data.data(), &head, sizeof(HEAD));
    memcpy(data.data() + sizeof(HEAD), &req, sizeof(req));

    qDebug() << "DoctorOrder 发送排班查询:" << dept << m_monday.toString("yyyy-MM-dd");
    emit get_guard_info(data, send_size);
}

void DoctorOrder::applyRoster()
{
    for (int r = 0; r < m_docNames.size(); ++r) {
        for (int d = 0; d < 7; ++d) {
            for (int k = 0; k < 3; ++k) {
                int idx = r * 21 + d * 3 + k;
                if (idx >= m_cards.size())
                    return;
                DoctorSlotCard *card = m_cards[idx];
                const GUARD_REPIX_T &slot = m_info[k][d];
                QString duty = QString::fromUtf8(slot.name).trimmed();
                bool mine = (!slot.isfree) && (duty == m_docNames[r]);
                if (mine) {
                    // 该时段由本行医生值班：显示科室信息并置蓝
                    QString departText = QString::fromUtf8(slot.depart).trimmed();
                    QString body = departText.isEmpty()
                                       ? periodNames[k]
                                       : QString("%1\n坐诊").arg(departText);
                    card->setText(body);
                    card->setFree(false);
                }
                else {
                    card->setText(periodNames[k]);
                    card->setFree(true);
                }
            }
        }
    }
}

void DoctorOrder::onCardToggled(int r, int d, int k)
{
    if (r < 0 || r >= m_docNames.size())
        return;

    GUARD_REPIX_T &slot = m_info[k][d];
    QString cur = slot.isfree ? QString() : QString::fromUtf8(slot.name).trimmed();

    if (!slot.isfree && cur == m_docNames[r]) {
        // 已由本行医生值班 -> 取消
        memset(&slot, 0, sizeof(slot));
        slot.isfree = true;
    }
    else {
        // 指派本行医生值班该(天,时段)；同时自然顶替原来可能的其他医生
        memset(&slot, 0, sizeof(slot));
        slot.isfree = false;
        slot.time = k;
        slot.id = (r < m_docIds.size()) ? m_docIds[r] : CData::m_id;
        copyCStr(slot.name, sizeof(slot.name), m_docNames[r]);
        copyCStr(slot.depart, sizeof(slot.depart), m_deptCombo->currentText());
        copyCStr(slot.date, sizeof(slot.date), m_monday.addDays(d).toString("yyyy-MM-dd"));
    }
    if (r < m_modified.size())
        m_modified[r].flag[d * 3 + k] = true; // 只要被点过即标记（改回原样也算修改过）

    qDebug() << "卡片点击 行医生" << m_docNames[r]
             << "周" << (d + 1) << "时段" << periodNames[k]
             << (slot.isfree ? "取消" : "指派");
    applyRoster();
}

void DoctorOrder::dumpSchedule()
{
    qDebug() << "===== 排班表修改项(保存修改，只打印修改过的) =====";
    for (int r = 0; r < m_docNames.size() && r < m_modified.size(); ++r) {
        QString line;
        for (int d = 0; d < 7; ++d) {
            for (int k = 0; k < 3; ++k) {
                if (!m_modified[r].flag[d * 3 + k])
                    continue; // 没被点过的位置不打印
                const GUARD_REPIX_T &slot = m_info[k][d];
                QString duty = slot.isfree ? QString()
                                           : QString::fromUtf8(slot.name).trimmed();
                bool mine = (!slot.isfree) && (duty == m_docNames[r]);
                QString state;
                if (mine) {
                    QString depart = QString::fromUtf8(slot.depart).trimmed();
                    state = depart.isEmpty() ? QStringLiteral("已排") : depart + QStringLiteral("·已排");
                }
                else {
                    state = QStringLiteral("已取消");
                }
                line += QString(" 周%1%2(%3)").arg(d + 1).arg(periodNames[k]).arg(state);
            }
        }
        if (!line.isEmpty()) {
            int id = (r < m_docIds.size()) ? m_docIds[r] : -1;
            QString head = QString("医生: %1 id: %2").arg(m_docNames[r]).arg(id);
            qDebug().noquote() << head << line;
        }
    }
    qDebug() << "===== 打印完成 =====";
}

void DoctorOrder::saveInfo()
{
    dumpSchedule(); // 顺带打印修改项

    HEAD head;
    memset(&head, 0, sizeof(head));
    head.type = SERVICE_TYPE::REPIX_GUARD;
    head.is_fragment = false;

    const int single_pack = sizeof(GUARD_REPIX_T);
    // 上界：HEAD + 最多 21 个槽位记录（7天*3段）
    QByteArray send_data;
    send_data.resize(sizeof(HEAD) + 21 * single_pack);
    char *p = send_data.data();
    int pre = sizeof(HEAD);
    head.frag_total = 0;

    // (day,time) 只要被任意医生的修改标记置过位，就打包该槽位的最新记录
    for (int d = 0; d < 7; ++d) {
        for (int k = 0; k < 3; ++k) {
            bool dirty = false;
            for (int r = 0; r < m_modified.size(); ++r) {
                if (m_modified[r].flag[d * 3 + k]) {
                    dirty = true;
                    break;
                }
            }
            if (!dirty)
                continue;

            GUARD_REPIX_T &slot = m_info[k][d];
            CData::repix_info.push_back(slot);
            memcpy(p + pre, &slot, single_pack);
            pre += single_pack;
            head.frag_total++;
        }
    }

    head.len = head.frag_total * single_pack;
    memcpy(p, &head, sizeof(HEAD));

    if (head.frag_total <= 0) {
        qDebug() << "保存：无修改项，不发送";
        return;
    }
    qDebug() << "保存：发送 REPIX_GUARD，修改槽位数" << head.frag_total;
    emit save_guard_info(send_data, pre);
}

void DoctorOrder::flush_doctor()
{
    qDebug() << "DoctorOrder flush_doctor";
    if (CData::current_widget != this)
        return; // 不在本页时先不刷新，避免后台页重建

    m_docNames.clear();
    m_docIds.clear();
    for (const auto &doc : CData::selece_department_info) {
        QString name = QString::fromUtf8(doc.name);
        if (!name.isEmpty()) {
            m_docNames.push_back(name);
            m_docIds.push_back(doc.id);
        }
    }
    // 每位医生一个 21 位修改标记，随医生名单重建并清零
    m_modified.resize(m_docNames.size());
    for (DocModified &row : m_modified)
        memset(row.flag, 0, sizeof(row.flag));

    rebuildContent();
    applyRoster(); // 医生行就绪后按 m_info 重绘（含刚返回的排班）
}

void DoctorOrder::flush_table()
{
    qDebug() << "DoctorOrder flush_table";
    if (CData::current_widget != this)
        return;

    memcpy(m_info, &CData::m_get_cards.guards, sizeof(m_info));
    applyRoster();
}

void DoctorOrder::openCopyDialog()
{
    if (m_deptCombo->currentIndex() <= 0) {
        QMessageBox::information(this, "批量复制排班",
                                 "请先选择科室并加载排班，再进行批量复制。");
        return;
    }

    CopyScheduleDialog dlg(m_monday, m_deptCombo->currentText(), this);
    if (dlg.exec() == QDialog::Accepted)
        startCopyWeeks(dlg.copyWeeks());
}

void DoctorOrder::startCopyWeeks(int weeks)
{
    if (weeks <= 0)
        return;

    // 先把当前展示周的排班整体快照下来，发送过程中翻页/改卡也不受影响
    GUARD_REPIX_T src[3][7];
    memcpy(src, m_info, sizeof(src));

    const int single_pack = sizeof(GUARD_REPIX_T);
    m_copyQueue.clear();

    // ===== 方案A：行驱动直取 id，不做“按名字在表里反查 id” =====
    // 界面把每个值班格画在某一个医生行里，而每行医生的 id 是 SELECT_DOCTOR 直接下发的
    // (m_docIds)。先把每格归属到某一行解析出来（同一源周每周都一样，只解析一次）：
    //   owner[d][k] >=0  第 d 天第 k 时段由“本行医生”值班，复制时 id 直接取 m_docIds[owner]
    //                -1  空档（目标周发 isfree=true 占位清除）
    //                -2  值班医生在本科室医生表里找不到，或两行医生同名(重名无法区分身份)
    //                    → 该格宁可不发（目标周保持原样），也绝不发一个 id=0 的值班出去
    int owner[7][3];
    int occupiedTotal = 0;      // 源周值班格总数
    QStringList skipNames;      // 被跳过(无法归属/重名)的值班医生名，最多记 8 个

    for (int d = 0; d < 7; ++d) {
        for (int k = 0; k < 3; ++k) {
            const GUARD_REPIX_T &cell = src[k][d];
            if (cell.isfree) {
                owner[d][k] = -1;
                continue;
            }

            ++occupiedTotal;
            QString cellName = QString::fromUtf8(cell.name).trimmed();
            int hit = -1;       // 匹配到的医生行
            bool dup = false;   // 多行医生同名都匹配 -> 重名，无法判断是哪一个
            for (int r = 0; r < m_docNames.size() && r < m_docIds.size(); ++r) {
                if (m_docNames[r].trimmed() != cellName)
                    continue;
                if (hit >= 0) { dup = true; break; }
                hit = r;
            }
            if (dup || hit < 0) {
                owner[d][k] = -2;
                if (skipNames.size() < 8 && !skipNames.contains(cellName))
                    skipNames << cellName;
            }
            else {
                owner[d][k] = hit;
            }
        }
    }

    if (!skipNames.isEmpty())
        qDebug().noquote() << "批量复制警告：以下值班槽无法确定医生身份，已跳过不发(绝不发 id=0): "
                           << skipNames.join(", ");

    // ===== 逐周打包：每周一个 REPIX_GUARD 包，可归属的格子全部带上 =====
    QStringList rowIdZeroNames; // 医生行 id 本身为 0（服务端医生表下发的 id 就是 0）
    for (int w = 1; w <= weeks; ++w) {
        QDate targetMonday = m_monday.addDays(7 * w);

        QByteArray data;
        data.resize(sizeof(HEAD) + 21 * single_pack);
        memset(data.data(), 0, data.size());

        HEAD head;
        memset(&head, 0, sizeof(head));
        head.type = SERVICE_TYPE::REPIX_GUARD;
        head.is_fragment = false;

        char *p = data.data();
        int pre = sizeof(HEAD);
        int fragCount = 0;
        for (int d = 0; d < 7; ++d) {
            for (int k = 0; k < 3; ++k) {
                int r = owner[d][k];
                if (r == -2)            // 无法归属/重名：本格不发，目标周保持原样
                    continue;

                // 逐槽显式重建：date=目标日、time=时段 k 必须写对，
                // 空槽是清除指令，若沿用源空槽里脏的 time 字段可能清错时段的格子
                GUARD_REPIX_T slot;
                memset(&slot, 0, sizeof(slot));
                copyCStr(slot.date, sizeof(slot.date),
                         targetMonday.addDays(d).toString("yyyy-MM-dd"));
                slot.time = k;
                if (r >= 0) {
                    slot.isfree = false;
                    slot.id = (r < m_docIds.size()) ? m_docIds[r] : 0;  // 直接取本行医生 id
                    if (slot.id <= 0) {
                        QString nm = m_docNames[r].trimmed();
                        if (rowIdZeroNames.size() < 8 && !rowIdZeroNames.contains(nm))
                            rowIdZeroNames << nm;
                    }
                    copyCStr(slot.name, sizeof(slot.name), m_docNames[r].trimmed());
                    QString dept = QString::fromUtf8(src[k][d].depart).trimmed();
                    if (dept.isEmpty())
                        dept = m_deptCombo->currentText();
                    copyCStr(slot.depart, sizeof(slot.depart), dept);
                }
                else {
                    slot.isfree = true; // 空档：占位，整周覆盖时清除目标周对应格子
                }
                memcpy(p + pre, &slot, single_pack);
                pre += single_pack;
                ++fragCount;
            }
        }

        if (fragCount <= 0) {
            qDebug() << "批量复制：第" << w << "周没有任何可归属的槽，跳过不发包";
            continue;
        }
        head.frag_total = fragCount;
        head.len = fragCount * single_pack;
        memcpy(p, &head, sizeof(HEAD));
        data.resize(pre);   // 只保留实际用到的字节，别把尾部 0 当下一包头

        m_copyQueue.append(data);
        qDebug() << "批量复制：打包第" << w << "/" << weeks << "周，起于"
                 << targetMonday.toString("yyyy-MM-dd") << "，" << fragCount << "槽";
    }

    // 排查用汇总
    qDebug() << "批量复制汇总：源周值班格" << occupiedTotal << "个；跳过(找不到/重名)"
             << skipNames.size() << "位医生；医生行 id<=0 的" << rowIdZeroNames.size()
             << "位（医生表" << m_docNames.size() << "条）";
    if (!rowIdZeroNames.isEmpty())
        qDebug().noquote() << "  医生行 id 本身为 0 的医生(最多8个): " << rowIdZeroNames.join(", ");

    // 发送期间锁定相关控件，逐包由定时器节奏发出（间隔等同每包之间等待）
    m_copyBtn->setEnabled(false);
    m_saveBtn->setEnabled(false);
    m_prevBtn->setEnabled(false);
    m_nextBtn->setEnabled(false);
    m_deptCombo->setEnabled(false);
    m_copyTimer->start();
}

void DoctorOrder::sendNextCopyPack()
{
    if (m_copyQueue.isEmpty()) {
        m_copyTimer->stop();
        m_copyBtn->setEnabled(true);
        m_saveBtn->setEnabled(true);
        m_prevBtn->setEnabled(true);
        m_nextBtn->setEnabled(true);
        m_deptCombo->setEnabled(true);
        qDebug() << "批量复制：全部周发送完毕";
        return;
    }

    QByteArray data = m_copyQueue.takeFirst();
    qDebug() << "批量复制：发送一周 REPIX_GUARD，字节" << data.size();
    emit save_guard_info(data, data.size());
}
