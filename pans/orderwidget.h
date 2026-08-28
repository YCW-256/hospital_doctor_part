#ifndef ORDERWIDGET_H
#define ORDERWIDGET_H

#include <QWidget>
#include "childs/customcard.h"
#include "../MyTcp/protecol.h"
namespace Ui {
class OrderWidget;
}

class OrderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OrderWidget(QWidget *parent = nullptr);
    ~OrderWidget();

private:
    Ui::OrderWidget *ui;

    CustomCard* myCards[3][7];

    QLabel* dateLabels[7];

    void init_guard_card();

    void init_connect();

    bool is_repix[3][7];

    GUARD_REPIX_T current_info[3][7];
public slots:
    void flush_doctor();

private slots:
    void on_comboBox_department_currentIndexChanged(int index);

    void on_comboBox_doctor_currentIndexChanged(int index);



signals:

    void get_doctor_info(const QByteArray data,int send_size);

    void push_card_to_deal();

};

#endif // ORDERWIDGET_H
