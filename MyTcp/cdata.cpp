#include "cdata.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
vector<int>CData::devices;

QString CData::ip="";
int CData::port=0;
int CData::current_width=500;
int CData::current_height=200;
int CData::m_id=-1;
vector<APP_INFO> CData::app_info{};
CData::CData() {


}


bool CData:: readJsonFile()
{
    QString config_path=":/configs/total_config.josn";
    QFile file(config_path);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug()<<"打开文件失败:"<<file.errorString();
        return false;
    }
    //读取全部字节
    QByteArray jsonData = file.readAll();
    file.close();

    //解析文档
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if(doc.isNull())
    {
        qDebug()<<"json解析失败";
        return false;
    }

    //顶层是对象 {}
    QJsonObject obj = doc.object();
    //读取普通字段
    CData::ip= obj["ip"].toString();
    CData::port= obj["port"].toInt();
    //读取数组 hobby:[...]
    QJsonArray arr = obj["CURRENT_SIZE"].toArray();
    CData::current_width=arr.at(0).toInt();
    CData::current_height=arr.at(1).toInt();
    qDebug()<<"ip："<<CData::ip <<"port："<< CData::port <<"width"<< CData::current_width <<"height"<<CData::current_height;

    return true;
}










void CData::init()
{
    readJsonFile();
    // if(read_devices_id()){
    //     qDebug()<<"读取设备id成功";
    // }

}




bool CData::read_devices_id()
{
    // ifstream fd(config_path);  // 假设文件名为 example.txt
    // if(!fd.is_open()){
    //     qDebug()<<"文件打开失败";
    //     return false;
    // }
    // devices.resize(0);
    // int id;
    // while(fd>>id){
    //     devices.push_back(id);
    //     qDebug()<<"读取设备id："<<id;
    // }
    return true;
}
