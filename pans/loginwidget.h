#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include "MyTcp/socketlink.h"
namespace Ui {
class LoginWidget;
}

class LoginWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWidget(SocketLink *socket,QWidget *parent = nullptr);
    ~LoginWidget();

private:
    Ui::LoginWidget *ui;
    SocketLink *m_socket;
    void init_connect();
signals:
    void to_login_ok(const QByteArray data,int len);
    void send_ok(const QByteArray data,int len);
    void loginSuccess();
};

#endif // LOGINWIDGET_H
