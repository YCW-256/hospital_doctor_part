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
#include "../Task/gettongueimgtask.h"
#include "../Tool/myutils.h"

// 单个包 body 的长度上限（防脏头）：目前最大的响应体就是舌苔图片分片 IMG_T（8312B），
// 比它大的一定是两端口径对不上/缓冲错位，不能拿野长度去拷。
static const int kMaxBodySize = static_cast<int>(sizeof(IMG_T));

// 这个头“像不像正经头”：类型必须是已知的 SERVICE_TYPE，长度要在合理范围内。
// 服务端发包残了（例如 send() 只写了一半就接着发下一包）时，客户端会从半包中间读到
// 一堆像素字节当包头，靠这个判定识别出来。
static bool isPlausibleHead(const HEAD &h)
{
    const int t = static_cast<int>(h.type);
    return t >= 0 && t <= static_cast<int>(SERVICE_TYPE::GET_TONGUE_IMG)
           && h.len >= 0 && h.len <= kMaxBodySize;
}

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
    // 1) 把这次 socket 上读到的字节全部追加进收包缓冲。
    //    **缓冲是动态 QByteArray**（原来是 char[4096]）：舌苔图片一片就有
    //    HEAD(24)+IMG_T(8312)=8336 字节，固定 4096 的老写法 memcpy 会直接写越界。
    const QByteArray chunk = socket->readAll();
    if (!chunk.isEmpty())
        buf_data.append(chunk);

    if (buf_data.size() < static_cast<int>(sizeof(HEAD)))
        return;   // 连头都没收全，等下一次 readyRead

    HEAD head;
    memcpy(&head, buf_data.constData(), sizeof(HEAD));

    // 2) 脏头防护：服务端某一次 send() 只写了一半就接着发下一包时（socket 发送缓冲满、
    //    非阻塞 send 的返回值没判），客户端会从半包中间读起，把像素字节当包头。
    //    处理办法是**向前逐字节找一个像样的头**，只丢掉前面的垃圾；老写法把整个缓冲清零，
    //    等于一个残包把后面几十个正常包一起带下水（2026-09-12 实测踩过）。
    //    对齐本身不算“解包”，做完就返回，下一趟再解。
    if (!isPlausibleHead(head)) {
        int scan = 0;
        const int lim = buf_data.size() - static_cast<int>(sizeof(HEAD));
        for (; scan <= lim; ++scan) {
            HEAD cand;
            memcpy(&cand, buf_data.constData() + scan, sizeof(HEAD));
            if (isPlausibleHead(cand))
                break;
        }
        if (scan > lim) {   // 缓冲里没有能对齐的头，整段丢掉等下一次
            qDebug() << "协议头异常且缓冲内无可对齐的头，清空收包缓冲 type" << head.type
                     << "len" << head.len << "缓冲" << buf_data.size();
            buf_data.clear();
            return;
        }
        qDebug() << "协议头异常，丢弃" << scan << "字节重新对齐（脏头 type" << head.type
                 << "len" << head.len << "）";
        buf_data.remove(0, scan);   // scan>=1，不会原地打转
        if (buf_data.size() >= static_cast<int>(sizeof(HEAD)))
            QTimer::singleShot(0, this, [this]() { recv_data(); });
        return;
    }

    if (buf_data.size() < static_cast<int>(sizeof(HEAD)) + head.len)
        return;   // 半包：留在缓冲里，等下一次 readyRead 凑齐

    // 3) 一次只解一个包（注意：Task 的构造函数收的是 QByteArray&（非 const），这里不能加 const）
    QByteArray body = buf_data.mid(static_cast<int>(sizeof(HEAD)), head.len);
    buf_data.remove(0, static_cast<int>(sizeof(HEAD)) + head.len);

    qDebug()<<"type"<<head.type<<"head.len"<<head.len
             <<"缓冲剩余"<<buf_data.size();

    if(head.type==SERVICE_TYPE::DOCTOR_LOGIN){
        LoginTask *task;
        task=new LoginTask(head.len,body,nullptr);
        task->execute();
        if(task->is_success) {emit login_success();}
        delete task;
    }
    else if(head.type==SERVICE_TYPE::DOCTOR_APP_INFO){
        GetInfoTask *task;
        task=new GetInfoTask(head.len,body,nullptr);
        task->execute();
        emit get_app_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::SELECT_DOCTOR){
        GetDepartmentDoctorTask *task;
        task=new GetDepartmentDoctorTask(head.len,body,nullptr);
        task->execute();
        emit get_doctor_info_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_GUARD){
        GetGuardTask *task;
        task=new GetGuardTask(head.len,body,nullptr);
        task->execute();
        emit get_guard_info_success();
        qDebug()<<"发送get_guard_info_success";
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_MEDICAL_RECORD){
        // 第一套：病历列表（GetMedicalRecordTask 已把结果写进 CData::medical_record_list / _total）
        GetMedicalRecordTask *task;
        task=new GetMedicalRecordTask(head.len,body,nullptr);
        task->execute();
        emit get_medical_record_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_MEDICAL_RECORD_DETAIL){
        // 第二套：病历详情（按 record_id 存进 CData::medical_record_details）
        GetMedicalRecordDetailTask *task;
        task=new GetMedicalRecordDetailTask(head.len,body,nullptr);
        task->execute();
        emit get_medical_record_detail_success();
        delete task;
    }
    else if(head.type==SERVICE_TYPE::GET_TONGUE_IMG){
        // 舌苔图片：服务端把一张图切成若干 IMG_T 分片发（一片 8192B 像素），
        // GetTongueImgTask 内部按分片 index 落位攒齐、收齐才拼成 QImage 写 CData；
        // 中途的分片不发信号，免得界面拿着半张图乱刷。
        GetTongueImgTask *task;
        task=new GetTongueImgTask(head.len,body,nullptr);
        task->execute();
        if(task->is_complete)
            emit get_tongue_img_success();
        delete task;
    }

    // 4) 缓冲里往往还躺着好几个整包（一次 readyRead 常带回 7~8 片图片）。
    //    一次只解一个的话，最后一片到齐后就再没有 readyRead 可等，尾巴会永远卡在缓冲里
    //    （图片就差最后几片拼不出来）。所以这里补排一趟自己：走事件循环、不递归、不阻塞界面；
    //    若剩下的只是半包，下一趟解不出来就自然停住，不会空转。
    if (buf_data.size() >= static_cast<int>(sizeof(HEAD)))
        QTimer::singleShot(0, this, [this]() { recv_data(); });
}





