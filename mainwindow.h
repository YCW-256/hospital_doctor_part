#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "MyTcp/socketlink.h"
#include "pans/loginwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void init_connect();

    void init_task_connect();

signals:
    void sendok(QByteArray,int size);
private:
    Ui::MainWindow *ui;

    SocketLink * m_socket;

    LoginWidget* login_widget;


};
#endif // MAINWINDOW_H
