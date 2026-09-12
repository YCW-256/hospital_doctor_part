#ifndef SOCKETLINK_H
#define SOCKETLINK_H

#include <QObject>
#include <QTcpSocket>
#include "protecol.h"
#include "QTimer"
#include "QByteArray"

// 收包缓冲不再用固定 char[BUF_SIZE]：舌苔图片一片就是 HEAD(24)+IMG_T(8312)=8336 字节，
// 固定 4096 的缓冲一 memcpy 就写越界。改成动态 QByteArray（见 recv_data）。
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
    //--------------------------缓存区元素----------------------------
    // 收包缓冲：已收到但还没解出来的字节。每轮 recv_data 解析完整包、把用掉的字节从头部 remove 掉，
    // 半包留在里面等下一次 readyRead（跨 readyRead 的粘包/拆包都靠它）。
    QByteArray buf_data;



signals:
    void sendok();
    void recvok(int id, int wet, int temperature);
    void warning(int id);

    void login_success();

    void get_app_success();

    void get_doctor_info_success();

    void get_guard_info_success();

    //病历【第一套·列表】：CData::medical_record_list / medical_record_total 已刷新
    void get_medical_record_success();

    //病历【第二套·详情】：CData::medical_record_details[record_id] 已写入
    void get_medical_record_detail_success();

    //舌苔图片：**整张图收齐**（多分片全部到齐并拼成 QImage）才发，
    // 此时 CData::tongue_image / tongue_image_patient_id 已是新值；中途的分片不发信号
    void get_tongue_img_success();

public slots:
    void onConnected();

    void onReadyRead();

    //发送数据
    void send_data(QByteArray send_buf,int size);
    //接收数据
    void recv_data();
};

#endif // SOCKETLINK_H
