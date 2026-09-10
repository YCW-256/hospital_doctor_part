#include "socketlink.h"
#include <string.h>
#include "CData.h"
#include <QThread>
#include "../Task/logintask.h"
#include "../Task/getinfotask.h"
#include "../Task/getdepartmentdoctortask.h"
#include "../Task/getguardtask.h"
#include "../Task/getmedicalrecordtask.h"
#include "../Task/getmedicalrecorddetailtask.h"
#include "../Tool/myutils.h"
SocketLink::SocketLink(QObject *parent)
    : QObject{parent}
{
    this->socket=new QTcpSocket(this);
    timer.setInterval(1000);
    connect(socket,&QTcpSocket::connected,this,&SocketLink::onConnected);
    connect(&timer,&QTimer::timeout,this,&SocketLink::onConnected);
    //connect(socket,&QTcpSocket::readyRead,this,&SocketLink::onReadyRead);
    connect(socket,&QTcpSocket::readyRead,this,&SocketLink::recv_data);
    //初始化缓存区
    memset(this->buf_data,0,sizeof(this->buf_data));
    p_use=0;
    p_now=0;
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
    qDebug()<<"..................................................................................................................................................................";
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
    qDebug()<<"读";
    HEAD head;
    QByteArray recv_head= socket->readAll();
    char *p=recv_head.data();
    //缓存区
    int read_len=recv_head.size();
    if(read_len>BUF_SIZE-p_now){
        MyUtils::ruleBuf( buf_data, p_use, p_now);
        qDebug()<<"规范";

    }
    memcpy(buf_data+p_now,p,read_len);
    p_now+=read_len;

    if(p_now-p_use<sizeof(HEAD))
        return;
    memcpy(&head,buf_data+p_use,sizeof(HEAD));
    p_use+=sizeof(HEAD);
    if(p_now-p_use<head.len){
        p_use-=sizeof(HEAD);
        return;
    }
    QByteArray recv_data;
    recv_data.resize(head.len);
    char* recv_p=recv_data.data();
    memcpy(recv_p,buf_data+p_use,head.len);
    p_use+=head.len;


    //qDebug() << "期望读取 body:" << head.len << "，实际读取:" << recv_data.size();
    qDebug()<<"type"<<head.type;
    qDebug()<<"p_use"<<p_use<<"p_now"<<p_now;
    qDebug()<<"head.len"<<head.len<<"sizeof(GET_GUARD_RESP)"<<sizeof(GET_GUARD_RESP)<<"head size"<<sizeof(HEAD)<<"total "<<head.len+sizeof(head);

    //最终兜底
    if(p_use!=p_now){
        p_use=0;
        p_now=0;
    }


    if(head.type==SERVICE_TYPE::DOCTOR_LOGIN){
        LoginTask *task;
        task=new LoginTask(head.len,recv_data,nullptr);
        task->execute();
        if(task->is_success) {emit login_success();}
        delete task;
    }
    else if(head.type==SERVICE_TYPE::DOCTOR_APP_INFO){
        GetInfoTask *task;
        task=new GetInfoTask(head.len,recv_data,nullptr);
        task->execute();
        emit get_app_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::SELECT_DOCTOR){
        GetDepartmentDoctorTask *task;
        task=new GetDepartmentDoctorTask(head.len,recv_data,nullptr);
        task->execute();
        emit get_doctor_info_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_GUARD){
        GetGuardTask *task;
        task=new GetGuardTask(head.len,recv_data,nullptr);
        task->execute();
        emit get_guard_info_success();
        qDebug()<<"发送get_guard_info_success";
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_MEDICAL_RECORD){
        // 第一套：病历列表（GetMedicalRecordTask 已把结果写进 CData::medical_record_list / _total）
        GetMedicalRecordTask *task;
        task=new GetMedicalRecordTask(head.len,recv_data,nullptr);
        task->execute();
        emit get_medical_record_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_MEDICAL_RECORD_DETAIL){
        // 第二套：病历详情（按 record_id 存进 CData::medical_record_details）
        GetMedicalRecordDetailTask *task;
        task=new GetMedicalRecordDetailTask(head.len,recv_data,nullptr);
        task->execute();
        emit get_medical_record_detail_success();
        delete task;
    }



}





