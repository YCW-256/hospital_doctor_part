#include "selbtn.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QAbstractButton>

SelBtn::SelBtn( QWidget *parent)
    : QWidget(parent)
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    QStringList items={"今天", "本周", "本月"};
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setStyleSheet("SelBtn { background-color:transparent; border: 1px solid #E5E5E5; border-radius: 8px; }");
    //this->setStyleSheet("SelBtn { background: transparent; border: none; }");
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);

    QString btnStyle = R"(
        QPushButton {
            background: transparent;
            border: none;
            padding: 8px 20px;
            font-size: 14px;
            color: #666666;
            border-bottom: 2px solid transparent;
        }
        QPushButton:hover {
            color: #1E70BF;
        }
        QPushButton:checked {
            color: #1E70BF;
            font-weight: bold;
            border-bottom: 2px solid #1E70BF;
        }
    )";

    for (int i = 0; i < items.size(); ++i) {
        if (i > 0) {
            QFrame *line = new QFrame(this);
            line->setFrameShape(QFrame::VLine);
            line->setFixedWidth(1);
            line->setStyleSheet("background-color: #E0E0E0; border: none;");
            layout->addWidget(line);
        }
        QPushButton *btn = new QPushButton(items.at(i), this);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(btnStyle);
        m_buttonGroup->addButton(btn, i);
        layout->addWidget(btn, 1);
    }

    if (!items.isEmpty()) {
        m_buttonGroup->button(0)->setChecked(true);
    }

    connect(m_buttonGroup, &QButtonGroup::idClicked, this, [this](int id) {
        emit currentIndexChanged(id);
    });
}

void SelBtn::setCurrentIndex(int index)
{
    if (QAbstractButton *btn = m_buttonGroup->button(index)) {
        btn->setChecked(true);
    }
}

int SelBtn::currentIndex() const
{
    return m_buttonGroup->checkedId();
}