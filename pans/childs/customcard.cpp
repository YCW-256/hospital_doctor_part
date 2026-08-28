#include "CustomCard.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QDebug>

CustomCard::CustomCard(const QString &department, const QString &timeSlot, const QString &event,const QString &name,QWidget *parent)
    : QWidget(parent)
{


    isfree=true;
    // 修改为高大于宽的比例（宽140，高180）
    //setFixedSize(140, 180);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("QWidget { background-color: white; border: 1px solid #A9D0F5; border-radius: 10px; }");

    // 只使用一个垂直布局，实现“元素一行一个”
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 20, 15, 20);
    mainLayout->setSpacing(15); // 元素之间的间距

    // 1. 科室（放最上面，字体加粗）
    departmentLabel = new QLabel(department, this);
    departmentLabel->setAlignment(Qt::AlignCenter);
    departmentLabel->setStyleSheet("border: none; font-weight: bold; font-size: 16px; color: #333;");
    mainLayout->addWidget(departmentLabel);

    // 2. 事件（中间，胶囊样式）
    eventLabel = new QLabel(event, this);
    QString bgColor = (event == "手术") ? "#FFEBEB" : "#E0F7FA";
    QString fontColor = (event == "手术") ? "#E57373" : "#00ACC1";
    eventLabel->setAlignment(Qt::AlignCenter);
    // eventLabel->setStyleSheet(QString("border: none; background-color: %1; color: %2; border-radius: 12px; padding: 4px 10px; font-weight: bold; font-size: 12px;")
    //                               .arg(bgColor, fontColor));

    eventLabel->setStyleSheet(QString("border: none;  border-radius: 12px; padding: 4px 10px; font-weight: bold; font-size: 12px;")
                                        );

    mainLayout->addWidget(eventLabel, 0, Qt::AlignCenter); // 居中显示

    // 3. 时间（放在最下面）
    timeLabel = new QLabel(timeSlot, this);
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setStyleSheet("border: none; font-size: 14px; color: #555;");
    mainLayout->addWidget(timeLabel);
    //4 name
    nameLabel = new QLabel(name, this);
    nameLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setStyleSheet("border: none; font-size: 14px; color: #555;");
    mainLayout->addWidget(nameLabel);
    // 添加弹簧，让中间有个过渡感，保证上下间距匀称
    mainLayout->addStretch();

    // 关键：让内部的文字标签不拦截鼠标事件，使点击卡片任意位置都能触发父级的点击事件
    departmentLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    eventLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    timeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
}

void CustomCard::switch_color()
{
    if(isfree){
        setStyleSheet("QWidget { background-color: #8BB5EA; border: 1px solid #A9D0F5; border-radius: 10px; }");// 备选颜色#8BB5EA
    }
    else{
        setStyleSheet("QWidget { background-color: white; border: 1px solid #A9D0F5; border-radius: 10px; }");
    }
}



void CustomCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        qDebug() << "点击日程卡";
        //switch_color();

        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void CustomCard::to_deal_leave(const QString content[])
{
    switch_color();
    if(!isfree){
        setCardInfo("","","","");
    }
    else{
        setCardInfo(content[0],content[1],content[2],content[3]);
    }

    isfree=1-isfree;


}


void CustomCard::setCardInfo(const QString &department, const QString &timeSlot, const QString &event, const QString &name)
{
    departmentLabel->setText(department);
    timeLabel->setText(timeSlot);
    eventLabel->setText(event);
    nameLabel->setText(name);
}

bool CustomCard::getIsfree()
{
    return isfree;
}

void CustomCard::setFree(bool flag)
{
    this->isfree=flag;
}