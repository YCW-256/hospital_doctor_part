#include "medicalcardwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include "Tool/myutils.h"
MedicalCardWidget::MedicalCardWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void MedicalCardWidget::initUI()
{
    // 【关键】开启样式表背景属性，否则圆角不会生效！
    this->setAttribute(Qt::WA_StyledBackground, true);

    // 整体背景：浅蓝色，带圆角
    this->setStyleSheet(
        "MedicalCardWidget {"
        "    background-color: #B7D4F2;"  // 卡片底色
        "    border-radius: 15px;"        // 圆角
        "}"
        );

    // 1. 主布局（左右结构）
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // 2. 左侧图标区（图片里的卡片+时钟符号）
    // 实际项目中建议用 QLabel 放 QPixmap 图片，这里用文字模拟一个占位框
    m_iconLabel = new QLabel("📋", this);
    MyUtils::setLabelImg(m_iconLabel,":/icons/sys/dangan.png", 60);
    m_iconLabel->setFixedSize(60, 60);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setStyleSheet(
        "QLabel {"
        "    border: 2px solid #4A90E2;" // 蓝色边框
        "    border-radius: 10px;"       // 图标框圆角
        "    font-size: 30px;"
        "    background-color: rgba(255, 255, 255, 0.3);"
        "}"
        );
    mainLayout->addWidget(m_iconLabel);

    // 3. 中间信息区（上下结构）
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    // 姓名
    m_nameLabel = new QLabel("王浩然", this);
    m_nameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #333333; border: none;");
    infoLayout->addWidget(m_nameLabel);

    // 科室
    m_deptLabel = new QLabel("皮肤科", this);
    m_deptLabel->setStyleSheet("font-size: 14px; color: #555555; border: none;");
    infoLayout->addWidget(m_deptLabel);

    // 医生
    m_doctorLabel = new QLabel("陈医生", this);
    m_doctorLabel->setStyleSheet("font-size: 14px; color: #555555; border: none;");
    infoLayout->addWidget(m_doctorLabel);

    // 分隔线
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #A0B4C8; background-color: #A0B4C8; height: 1px; border: none; margin: 5px 0;");
    infoLayout->addWidget(line);

    // 底部时间区（左右排列）
    QHBoxLayout *timeLayout = new QHBoxLayout();
    timeLayout->setSpacing(20);
    m_time1Label = new QLabel("14:00", this);
    m_time1Label->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333; border: none;");
    m_time2Label = new QLabel("14:00", this);
    m_time2Label->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333; border: none;");
    timeLayout->addWidget(m_time1Label);
    timeLayout->addWidget(m_time2Label);
    timeLayout->addStretch(); // 让时间靠左
    infoLayout->addLayout(timeLayout);

    infoLayout->addStretch(); // 将上方内容顶上去
    mainLayout->addLayout(infoLayout, 1); // 中间区占满剩余空间

    // 4. 右侧状态按钮区
    QVBoxLayout *statusLayout = new QVBoxLayout();
    statusLayout->setSpacing(10);

    // 上方按钮（已就诊）- 浅蓝色胶囊
    m_statusBtn1 = new QPushButton("已就诊", this);
    m_statusBtn1->setFixedSize(80, 30);
    m_statusBtn1->setCursor(Qt::PointingHandCursor);
    m_statusBtn1->setEnabled(false); // 展示用，禁止点击，防止出现点击浮层变色
    m_statusBtn1->setStyleSheet(
        "QPushButton {"
        "    background-color: #A0C4FF;"  // 浅蓝
        "    color: #1E3A8A;"
        "    border: none;"
        "    border-radius: 15px;"        // 胶囊形状
        "    font-size: 13px;"
        "}"
        );

    // 下方按钮（已确认）- 深蓝色胶囊
    m_statusBtn2 = new QPushButton("已确认", this);
    m_statusBtn2->setFixedSize(80, 30);
    m_statusBtn2->setCursor(Qt::PointingHandCursor);
    m_statusBtn2->setEnabled(false);
    m_statusBtn2->setStyleSheet(
        "QPushButton {"
        "    background-color: #1E70BF;"   // 深蓝
        "    color: white;"
        "    border: none;"
        "    border-radius: 15px;"
        "    font-size: 13px;"
        "}"
        );

    statusLayout->addWidget(m_statusBtn1);
    statusLayout->addStretch(); // 拉开两个按钮的间距
    statusLayout->addWidget(m_statusBtn2);

    mainLayout->addLayout(statusLayout);
}

void MedicalCardWidget::setInfo(const QString &name, const QString &department, const QString &doctor,
                                const QString &time1, const QString &time2,
                                bool hasVisited, bool confirmed)
{
    m_nameLabel->setText(name);
    m_deptLabel->setText(department);
    m_doctorLabel->setText(doctor);
    m_time1Label->setText(time1);
    m_time2Label->setText(time2);

    if (hasVisited) {
        m_statusBtn1->setText("已就诊");
        m_statusBtn1->setStyleSheet("background-color: #A0C4FF; color: #1E3A8A; border: none; border-radius: 15px;");
    } else {
        m_statusBtn1->setText("未就诊");
        m_statusBtn1->setStyleSheet("background-color: #E0E0E0; color: #757575; border: none; border-radius: 15px;");
    }

    if (confirmed) {
        m_statusBtn2->setText("已确认");
        m_statusBtn2->setStyleSheet("background-color: #1E70BF; color: white; border: none; border-radius: 15px;");
    } else {
        m_statusBtn2->setText("待确认");
        m_statusBtn2->setStyleSheet("background-color: #FFC107; color: #333; border: none; border-radius: 15px;");
    }
}