#ifndef CUSTOMCARD_H
#define CUSTOMCARD_H

#include <QWidget>

class QLabel;

class CustomCard : public QWidget
{
    Q_OBJECT

public:
    explicit CustomCard(const QString &department="内科101", const QString &timeSlot="下午", const QString &event="坐诊", QWidget *parent = nullptr);

    //CustomCard(QWidget *parent = nullptr);
signals:
    void clicked(); // 定义点击信号，方便外部连接处理业务

protected:
    void mousePressEvent(QMouseEvent *event) override; // 重写鼠标点击事件

private:
    QLabel *departmentLabel;
    QLabel *eventLabel;
    QLabel *timeLabel;
};

#endif // CUSTOMCARD_H