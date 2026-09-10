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

    // 首页“工作统计”图标按钮被点击（各窗口连到自己的工作统计页）
    void to_workstat_page();

    // 首页“查看病例”图标按钮（toolButton / dangan.png）被点击
    void to_record_page();


};

#endif // SYSWIDGET_H
