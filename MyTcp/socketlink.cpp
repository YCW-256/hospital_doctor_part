#include "socketlink.h"
#include <string.h>
#include "CData.h"
#include <QThread>
#include "../Task/logintask.h"
SocketLink::SocketLink(QObject *parent)
    : QObject{parent}
{
    this->socket=new QTcpSocket(this);
    timer.setInterval(1000);
    connect(socket,&QTcpSocket::connected,this,&SocketLink::onConnected);
    connect(&timer,&QTimer::timeout,this,&SocketLink::onConnected);
    //connect(socket,&QTcpSocket::readyRead,this,&SocketLink::onReadyRead);
    connect(socket,&QTcpSocket::readyRead,this,&SocketLink::recv_data);
}

void SocketLink::connectHost(const QString &hostName, quint16 port)
{

    this->socket->connectToHost(hostName,port);
}

void SocketLink::connectHost()
{
    QString hostName=CData::ip;
    int port =CData::port;
    this->socket->connectToHost(hostName,port);
}

void SocketLink::onConnected()
{
    // int size=CData::devices.size();
    // char *ptr=sendData.data();
    // ptr+=sizeof(HEAD);
    // int *int_ptr=(int*)ptr;
    // for(int i=0;i<size;i++){
    //     *int_ptr=CData::devices[i];
    //     qint64 len=this->socket->write(this->sendData);
    //     qDebug()<<"pop id"<<*int_ptr;
    //     if(len>0){
    //         qDebug()<<"pop success  "<<len;
    //         emit sendok();
    //         timer.start();
    //     }

    // }



}

void SocketLink::onReadyRead()
{
    // qDebug()<<"recv data trigger";
    // const int RESP_PACK_LEN = sizeof(HEAD)+sizeof(ENVIR_RESP);

    // // 持续读取缓冲区完整数据包，半包保留不丢弃
    // while(socket->bytesAvailable() >= RESP_PACK_LEN)
    // {
    //     QByteArray data = socket->read(RESP_PACK_LEN);
    //     HEAD head;

    //     ENVIR_RESP resp;
    //     memcpy(&head, data.data(), sizeof(HEAD));
    //     memcpy(&resp, data.data()+sizeof(HEAD), sizeof(ENVIR_RESP));
    //     if(head.type==SERVICE_TYPE::ENVIRONMENT){
    //         qDebug()<<"aaa";
    //     }

    //     qDebug()<<"湿度:"<<resp.wet <<"温度:"<<resp.temperature;
    //     qDebug()<<"total "<<sizeof(HEAD)+sizeof(ENVIR_RESP);
    //     int f=0;
    //     for (int i = 0; i < 12; ++i) {
    //         qDebug()<<resp.isClass[i];
    //         if(resp.isClass[i]){
    //             qDebug()<<"害虫: "<<i;
    //             f=1;
    //         }
    //     }
    //     if(f){
    //         qDebug()<<"有害虫";
    //         emit warning(resp.id);   // 通知界面：该设备有害虫，标黄警告
    //     }

    //     emit recvok(resp.id, resp.wet, resp.temperature);
    // }


}
//发送数据
void SocketLink::send_data(QByteArray send_buf,int size)
{
    qDebug()<<"【孩子pid】"<<QThread::currentThreadId();
    if(send_buf.isEmpty() || size <= 0)
        return;
    // if(socket->state() != QTcpSocket::ConnectedState)
    // {
    //     qDebug()<<"socket未连接，放弃发送";
    //     return;
    // }
    qint64 len=this->socket->write(send_buf,size);
    if(len>0){
        qDebug()<<"成功发送"<<len<<"字节"<<"状态:"<<QTcpSocket::ConnectedState;
    }
    else{
        qDebug()<<"发送失败";
    }

}

void SocketLink::recv_data()
{
    HEAD head;
    QByteArray recv_head= socket->read(sizeof(HEAD));
    char *p=recv_head.data();
    memcpy(&head,p,sizeof(HEAD));
    QByteArray recv_data= socket->read(head.len);
    BusinessTask *task;
    if(head.type==SERVICE_TYPE::DOCTOR_LOGIN){
        task=new LoginTask(head.len,recv_data,nullptr);
        task->execute();
    }


}





