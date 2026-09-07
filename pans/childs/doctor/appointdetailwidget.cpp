#include "appointdetailwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

AppointDetailWidget::AppointDetailWidget(const MeetRecord &rec, QWidget *parent)
    : QDialog(parent)
    , m_rec(rec)
    , m_patientLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_extraLabel(nullptr)
    , m_diagnosisEdit(nullptr)
    , m_prescriptionEdit(nullptr)
    , m_backBtn(nullptr)
    , m_doneBtn(nullptr)
{
    buildUi();
}

QString AppointDetailWidget::diagnosis() const
{
    return m_diagnosisEdit->toPlainText().trimmed();
}

QString AppointDetailWidget::treatPlan() const
{
    return m_prescriptionEdit->toPlainText().trimmed();
}

void AppointDetailWidget::buildUi()
{
    setWindowTitle(QStringLiteral("就诊详情"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("AppointDetailWidget { background: #F4FAFF; }");
    setFixedSize(460, 520);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(22, 18, 22, 14);
    root->setSpacing(10);

    QLabel *title = new QLabel(QStringLiteral("接诊信息"), this);
    title->setStyleSheet("border: none; font-size: 18px; font-weight: bold; color: #1E70BF;");
    root->addWidget(title);

    // 患者姓名（大字强调）
    m_patientLabel = new QLabel(this);
    m_patientLabel->setText(QString("患者：%1").arg(
        m_rec.patient_name.isEmpty() ? QStringLiteral("未知") : m_rec.patient_name));
    m_patientLabel->setStyleSheet("border: none; font-size: 17px; font-weight: bold; color: #333333;");
    root->addWidget(m_patientLabel);

    // 时间（日期与时段）
    m_timeLabel = new QLabel(this);
    m_timeLabel->setText(QString("时间：%1").arg(
        m_rec.time.isEmpty() ? QStringLiteral("未提供") : m_rec.time));
    m_timeLabel->setStyleSheet("border: none; font-size: 14px; color: #555555;");
    root->addWidget(m_timeLabel);

    // 次要信息：医生 / 预约号（供核对；缺省值用 - 占位）
    QString extra = QString("医生：%1    预约号：%2")
                        .arg(m_rec.doctor_name.isEmpty() ? QStringLiteral("-") : m_rec.doctor_name)
                        .arg(m_rec.meet_id >= 0 ? QString::number(m_rec.meet_id) : QStringLiteral("-"));
    m_extraLabel = new QLabel(extra, this);
    m_extraLabel->setStyleSheet("border: none; font-size: 13px; color: #888888;");
    root->addWidget(m_extraLabel);

    // 分隔线
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #B7D4F2; background-color: #B7D4F2; height: 1px; border: none; margin: 4px 0;");
    root->addWidget(line);

    // 诊断输入
    QLabel *diagLabel = new QLabel(QStringLiteral("诊断"), this);
    diagLabel->setStyleSheet("border: none; font-size: 14px; font-weight: bold; color: #333333;");
    root->addWidget(diagLabel);

    m_diagnosisEdit = new QTextEdit(this);
    m_diagnosisEdit->setPlaceholderText(QStringLiteral("请输入诊断……"));
    m_diagnosisEdit->setFixedHeight(90);
    m_diagnosisEdit->setStyleSheet(
        "QTextEdit {"
        "  background: #FFFFFF;"
        "  border: 1px solid #B7D4F2;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  padding: 6px;"
        "}"
        "QTextEdit:focus { border: 1px solid #2F80ED; }");
    root->addWidget(m_diagnosisEdit);

    // 处方输入
    QLabel *presLabel = new QLabel(QStringLiteral("处方"), this);
    presLabel->setStyleSheet("border: none; font-size: 14px; font-weight: bold; color: #333333;");
    root->addWidget(presLabel);

    m_prescriptionEdit = new QTextEdit(this);
    m_prescriptionEdit->setPlaceholderText(QStringLiteral("请输入处方……"));
    m_prescriptionEdit->setFixedHeight(90);
    m_prescriptionEdit->setStyleSheet(
        "QTextEdit {"
        "  background: #FFFFFF;"
        "  border: 1px solid #B7D4F2;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  padding: 6px;"
        "}"
        "QTextEdit:focus { border: 1px solid #2F80ED; }");
    root->addWidget(m_prescriptionEdit);

    root->addStretch();

    // 底部按钮：返回 | 完成
    QHBoxLayout *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    btnRow->addStretch();
    m_backBtn = new QPushButton(QStringLiteral("返回"), this);
    m_backBtn->setMinimumSize(92, 34);
    m_backBtn->setStyleSheet(
        "QPushButton{ background:#E6F0FA; color:#333333; border:1px solid #B7D4F2; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#D6E6F5; }");
    m_doneBtn = new QPushButton(QStringLiteral("完成"), this);
    m_doneBtn->setMinimumSize(92, 34);
    m_doneBtn->setStyleSheet(
        "QPushButton{ background:#2F80ED; color:#FFFFFF; border:none; border-radius:6px; font-size:14px; }"
        "QPushButton:hover{ background:#1E70BF; }");
    btnRow->addWidget(m_backBtn);
    btnRow->addWidget(m_doneBtn);
    root->addLayout(btnRow);

    connect(m_backBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_doneBtn, &QPushButton::clicked, this, &AppointDetailWidget::onDone);
}

void AppointDetailWidget::onDone()
{
    // 警告弹窗二次确认，防止误点“完成”
    int ret = QMessageBox::warning(
        this,
        QStringLiteral("完成确认"),
        QStringLiteral("确认完成本次就诊？\n\n提交后本次预约将标记为已就诊，并保存下方填写的诊断与处方。\n是否继续？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    printAll();
    accept();
}

void AppointDetailWidget::printAll() const
{
    qDebug().noquote() << QStringLiteral("================ 本次就诊信息 ================");
    qDebug().noquote() << QStringLiteral("meet_id     : %1").arg(m_rec.meet_id);
    qDebug().noquote() << QStringLiteral("doctor_id   : %1").arg(m_rec.doctor_id);
    qDebug().noquote() << QStringLiteral("patient_id  : %1").arg(m_rec.patient_id);
    qDebug().noquote() << QStringLiteral("doctor_name : %1").arg(m_rec.doctor_name);
    qDebug().noquote() << QStringLiteral("patient_name: %1").arg(m_rec.patient_name);
    qDebug().noquote() << QStringLiteral("time        : %1").arg(m_rec.time);
    qDebug().noquote() << QStringLiteral("诊断 diagnosis   : %1").arg(m_diagnosisEdit->toPlainText().trimmed());
    qDebug().noquote() << QStringLiteral("处方 prescription: %1").arg(m_prescriptionEdit->toPlainText().trimmed());
    qDebug().noquote() << QStringLiteral("=================================================");
}
