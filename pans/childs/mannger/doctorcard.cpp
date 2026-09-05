#include "doctorcard.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>

DoctorSlotCard::DoctorSlotCard(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_label(new QLabel(text, this))
    , m_free(true)
{
    setAttribute(Qt::WA_StyledBackground);
    // 扁瓦片：宽度容纳一天块，高度只放得下一行时段字
    setFixedSize(92, 40);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->setSpacing(0);

    m_label->setAlignment(Qt::AlignCenter);
    m_label->setStyleSheet("border: none; font-size: 12px; color: #6E96C6;");
    m_label->setAttribute(Qt::WA_TransparentForMouseEvents);
    lay->addWidget(m_label);

    applyStyle();
}

void DoctorSlotCard::applyStyle()
{
    // 类型选择器只作用于卡片本身，避免污染子 QLabel
    QString bg = m_free ? QStringLiteral("#FFFFFF") : QStringLiteral("#8BB5EA");
    setStyleSheet(QString("DoctorSlotCard { background: %1; border: 1px solid #A9D0F5; border-radius: 5px; }")
                      .arg(bg));
    // 占用(蓝底)时文字转白，保证可读
    QString color = m_free ? QStringLiteral("#6E96C6") : QStringLiteral("#FFFFFF");
    m_label->setStyleSheet(QString("border: none; font-size: 12px; font-weight: bold; color: %1;")
                               .arg(color));
}

void DoctorSlotCard::setText(const QString &text)
{
    m_label->setText(text);
}

QString DoctorSlotCard::text() const
{
    return m_label->text();
}

void DoctorSlotCard::setFree(bool free)
{
    m_free = free;
    applyStyle();
}

bool DoctorSlotCard::isFree() const
{
    return m_free;
}

void DoctorSlotCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked();
    QWidget::mousePressEvent(event);
}
