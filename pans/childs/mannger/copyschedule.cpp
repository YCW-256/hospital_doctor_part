#include "copyschedule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>

CopyScheduleDialog::CopyScheduleDialog(const QDate &srcMonday,
                                       const QString &department,
                                       QWidget *parent)
    : QDialog(parent)
    , m_srcMonday(srcMonday)
    , m_department(department)
    , m_weeks(1)
    , m_srcLabel(nullptr)
    , m_weeksSpin(nullptr)
    , m_targetLabel(nullptr)
    , m_okBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    buildUi();

    QString dept = m_department.isEmpty() ? QStringLiteral("未选科室") : m_department;
    m_srcLabel->setText(QString("源排班周（%1）：%2").arg(dept, rangeText(m_srcMonday)));
    refreshTarget();
}

int CopyScheduleDialog::copyWeeks() const
{
    return m_weeks;
}

void CopyScheduleDialog::buildUi()
{
    setWindowTitle(QStringLiteral("批量复制排班"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("CopyScheduleDialog { background: #F4FAFF; }");
    setFixedSize(470, 280);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 16, 20, 14);
    root->setSpacing(10);

    QLabel *title = new QLabel(QStringLiteral("批量复制当前排班"), this);
    title->setStyleSheet("border: none; font-size: 17px; font-weight: bold; color: #1E70BF;");
    root->addWidget(title);

    m_srcLabel = new QLabel(this);
    m_srcLabel->setStyleSheet("border: none; font-size: 13px; color: #333;");
    m_srcLabel->setWordWrap(true);
    root->addWidget(m_srcLabel);

    // N 选择行：把当前排班复制到未来的连续 N 周
    QHBoxLayout *spinRow = new QHBoxLayout;
    spinRow->setSpacing(8);
    QLabel *tip = new QLabel(QStringLiteral("把当前排班复制到未来连续"), this);
    tip->setStyleSheet("border: none; font-size: 13px; color: #555;");
    m_weeksSpin = new QSpinBox(this);
    m_weeksSpin->setRange(1, 26);
    m_weeksSpin->setValue(1);
    m_weeksSpin->setMinimumWidth(72);
    QLabel *tip2 = new QLabel(QStringLiteral("周"), this);
    tip2->setStyleSheet("border: none; font-size: 13px; color: #555;");
    spinRow->addWidget(tip);
    spinRow->addWidget(m_weeksSpin);
    spinRow->addWidget(tip2);
    spinRow->addStretch();
    root->addLayout(spinRow);

    m_targetLabel = new QLabel(this);
    m_targetLabel->setStyleSheet("border: none; font-size: 13px; font-weight: bold; color: #1E70BF;");
    m_targetLabel->setWordWrap(true);
    root->addWidget(m_targetLabel);

    QLabel *hint = new QLabel(
        QStringLiteral("复制将整周覆盖目标周的原有排班（源周空档也按空档写入）；发送后服务器不回包确认。"),
        this);
    hint->setStyleSheet("border: none; font-size: 12px; color: #999;");
    hint->setWordWrap(true);
    root->addWidget(hint);

    root->addStretch();

    // 底部按钮：取消 | 确认
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();
    m_cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    m_cancelBtn->setMinimumSize(92, 32);
    m_okBtn = new QPushButton(QStringLiteral("确认"), this);
    m_okBtn->setMinimumSize(92, 32);
    m_okBtn->setStyleSheet(
        "QPushButton{ background:#2F80ED; color:#FFFFFF; border:none; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#1E70BF; }");
    m_cancelBtn->setStyleSheet(
        "QPushButton{ background:#E6F0FA; color:#333333; border:1px solid #B7D4F2; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#D6E6F5; }");
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);

    connect(m_weeksSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CopyScheduleDialog::refreshTarget);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_okBtn, &QPushButton::clicked, this, &CopyScheduleDialog::onConfirm);
}

QString CopyScheduleDialog::rangeText(const QDate &monday)
{
    return QString("%1(周一) ～ %2(周日)")
        .arg(monday.toString(QStringLiteral("yyyy年M月d日")))
        .arg(monday.addDays(6).toString(QStringLiteral("M月d日")));
}

void CopyScheduleDialog::refreshTarget()
{
    m_weeks = m_weeksSpin->value();
    QDate lastMonday = m_srcMonday.addDays(7 * m_weeks);
    m_targetLabel->setText(
        QString("将整周复制到未来的连续 %1 周：\n"
                "从下一周 %2 开始，最后一周覆盖 %3 。")
            .arg(m_weeks)
            .arg(rangeText(m_srcMonday.addDays(7)))
            .arg(rangeText(lastMonday)));
}

void CopyScheduleDialog::onConfirm()
{
    refreshTarget(); // 保证 m_weeks 为最新

    QString msg = QString(
        "将把当前排班整周复制到接下来的连续 %1 周，目标周的原有排班会被覆盖。\n\n"
        "源排班周：%2\n"
        "最后复制周：%3\n\n"
        "是否继续？")
        .arg(m_weeks)
        .arg(rangeText(m_srcMonday))
        .arg(rangeText(m_srcMonday.addDays(7 * m_weeks)));

    int ret = QMessageBox::question(this, QStringLiteral("二次确认"), msg,
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if (ret == QMessageBox::Yes)
        accept();
}
