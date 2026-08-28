#ifndef CUSTOMCARD_H
#define CUSTOMCARD_H

#include <QWidget>

class QLabel;

class CustomCard : public QWidget
{
    Q_OBJECT

public:

    explicit CustomCard(const QString &department="", const QString &timeSlot="", const QString &event="", const QString &name="",QWidget *parent = nullptr);

    //CustomCard(QWidget *parent = nullptr);

    void switch_color();

    void setCardInfo(const QString &department, const QString &timeSlot, const QString &event, const QString &name);

    bool getIsfree ();

    void setFree(bool flag);
signals:
    void clicked(); // 定义点击信号，方便外部连接处理业务

protected:
    void mousePressEvent(QMouseEvent *event) override; // 重写鼠标点击事件

private:
    QLabel *departmentLabel;
    QLabel *eventLabel;
    QLabel *timeLabel;
    QLabel *nameLabel;
    bool isfree;

public slots:
    void to_deal_leave(const QString content[]);

};

#endif // CUSTOMCARD_H