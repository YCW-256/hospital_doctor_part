#ifndef GUARDWIDGET_H
#define GUARDWIDGET_H

#include <QWidget>
#include "childs/customcard.h"
namespace Ui {
class GuardWidget;
}

class GuardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GuardWidget(QWidget *parent = nullptr);
    ~GuardWidget();
    CustomCard* myCards[3][7];

    QLabel* dateLabels[7];

    void to_get_guards();
public slots:
    void flush_table();
private:
    Ui::GuardWidget *ui;

    void init_guard_card();

    void init_connect();

    void deal_card();

signals:
    void send_my_data(const QByteArray &data,int size);
};

#endif // GUARDWIDGET_H
