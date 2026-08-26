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
private:
    Ui::GuardWidget *ui;

    void init_guard_card();
};

#endif // GUARDWIDGET_H
