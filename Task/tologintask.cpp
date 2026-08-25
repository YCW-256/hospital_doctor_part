#include "tologintask.h"
#include <QDebug>
ToLoginTask::ToLoginTask(QObject *parent)
    : BusinessTask{parent}
{}

ToLoginTask::ToLoginTask(const QString name, const QString pwd, const QString ver_card, QObject *parent)
    : BusinessTask{parent}
{
    this->name=name;
    this->pwd=pwd;
    this->ver_card=ver_card;
}

void ToLoginTask::execute()
{

    send_data.resize(1024);
    char* p = send_data.data();
    HEAD head;
    DOCTOR_LOGIN_REQ req;
    head.type=SERVICE_TYPE::DOCTOR_LOGIN;
    head.frag_total=1;
    head.frag_index=1;
    head.is_fragment=0;
    head.len=sizeof(req);

    req.login_style=1;
    QByteArray b_name = name.toUtf8(); // 获取 UTF-8 字节
    QByteArray b_pwd = pwd.toUtf8(); // 获取 UTF-8 字节
    strcpy(req.account, b_name.constData());
    strcpy(req.pwd, b_pwd.constData());
    //完全包
    memcpy(p,&head,sizeof(HEAD));
    memcpy(p+sizeof(HEAD),&req,sizeof(req));
    qDebug()<<req.account<<req.pwd;
    len=sizeof(HEAD)+sizeof(req);
    //emit to_login_ok(send_data,sizeof(HEAD)+sizeof(req));
}
