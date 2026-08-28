#ifndef APPOINTWIDGET_H
#define APPOINTWIDGET_H
#include <QByteArray>
#include <QWidget>
enum SEL_TIME{
    DAY,
    WEEK,
    MOUNTH
};

namespace Ui {
class AppointWidget;
}

class AppointWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AppointWidget(QWidget *parent = nullptr);
    ~AppointWidget();

    void flush();

signals:
    void to_get_meet(const QByteArray data,int len);
private:
    void init_connect();

    Ui::AppointWidget *ui;

    void getAppInfo(int idx);


};

#endif // APPOINTWIDGET_H
