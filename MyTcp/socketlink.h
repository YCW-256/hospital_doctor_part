#ifndef SOCKETLINK_H
#define SOCKETLINK_H

#include <QObject>
#include <QTcpSocket>
#include "protecol.h"
#include "QTimer"
#include "QByteArray"
class SocketLink : public QObject
{
    Q_OBJECT
public:
    explicit SocketLink(QObject *parent = nullptr);

    void connectHost(const QString &hostName, quint16 port);

    QTcpSocket *socket;

    void connectHost();
private:

    HEAD head;
    GETENVIR_REQ req;
    QByteArray sendData;
    QTimer timer;
signals:
    void sendok();
    void recvok(int id, int wet, int temperature);
    void warning(int id);

    void login_success();

    void get_app_success();

    void get_doctor_info_success();

public slots:
    void onConnected();

    void onReadyRead();

    //发送数据
    void send_data(QByteArray send_buf,int size);
    //接收数据
    void recv_data();
};

#endif // SOCKETLINK_H
