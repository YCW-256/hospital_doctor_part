#include "logintask.h"
#include <qDebug>
LoginTask::LoginTask(QObject *parent)
    : BusinessTask::BusinessTask{parent}
{}

LoginTask::LoginTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask::BusinessTask(len,data,parent)
{

}

void LoginTask::execute()
{
    DOCTOR_LOGIN_RESP resp;
    char *p=data.data();
    memcpy(&resp,p,len);
    is_success=resp.result;
    qDebug()<<"是否成功"<<resp.result<<"id"<<resp.id;
}
