#ifndef APPOINTWIDGET_H
#define APPOINTWIDGET_H

#include <QWidget>

namespace Ui {
class AppointWidget;
}

class AppointWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AppointWidget(QWidget *parent = nullptr);
    ~AppointWidget();

private:
    Ui::AppointWidget *ui;
};

#endif // APPOINTWIDGET_H
