#ifndef MEDICALCARDWIDGET_H
#define MEDICALCARDWIDGET_H

#include <QWidget>

class QLabel;
class QPushButton;

class MedicalCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MedicalCardWidget(QWidget *parent = nullptr);

    // 提供接口，方便你动态修改卡片内容
    void setInfo(const QString &name, const QString &department, const QString &doctor,
                 const QString &time1, bool hasVisited, bool confirmed);

private:
    void initUI();

    // 控件成员
    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_deptLabel;
    QLabel *m_doctorLabel;
    QLabel *m_time1Label;
    // QLabel *m_time2Label; // 已移除
    QPushButton *m_statusBtn1;
    QPushButton *m_statusBtn2;
};

#endif // MEDICALCARDWIDGET_H