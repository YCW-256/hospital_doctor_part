#ifndef DOCTORCARD_H
#define DOCTORCARD_H

#include <QWidget>

class QLabel;
class QMouseEvent;

// 排班表里的一个时段格子卡。目前只需一个 label 显示时段名（上午/下午/晚上），
// 高度较扁，作为(医生×天)块内的三张无间距瓦片之一。
class DoctorSlotCard : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorSlotCard(const QString &text = QString(), QWidget *parent = nullptr);

    void setText(const QString &text);

    QString text() const;

    // free=true 空白(白底)；free=false 已占用(蓝底)，预留后续填数据用
    void setFree(bool free);

    bool isFree() const;

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void applyStyle();

    QLabel *m_label;
    bool m_free;
};

#endif // DOCTORCARD_H
