#ifndef SYSWIDGET_H
#define SYSWIDGET_H

#include <QWidget>

namespace Ui {
class SysWidget;
}

class SysWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SysWidget(QWidget *parent = nullptr);
    ~SysWidget();

private:
    Ui::SysWidget *ui;

    void init_myTable();

    void init_connect();
signals:

    void to_app_page();

    void to_guard_page();


};

#endif // SYSWIDGET_H
