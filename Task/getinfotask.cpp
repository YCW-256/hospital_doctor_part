#include "getinfotask.h"
GetInfoTask::GetInfoTask(QObject *parent)
    : BusinessTask{parent}
{

}

GetInfoTask::GetInfoTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask::BusinessTask(len,data,parent)
{
}

void GetInfoTask::execute()
{
    DOCTOR_APP_RESP resp;
    char *p=data.data();
    memcpy(&resp,p,sizeof(resp));
    CData::app_info.resize(resp.count);
    for (int i=0;i<resp.count;i++){
        qDebug()<<resp.PatientName[i]<<resp.time[i];
        CData::app_info[i].name=resp.PatientName[i];
        CData::app_info[i].time=resp.time[i];
        CData::app_info[i].state=resp.state[i];
        CData::app_info[i].meet_id=resp.meet_id[i];
        CData::app_info[i].doctorName=resp.DoctorName[i];
        CData::app_info[i].patient_id=resp.patient_id[i];
        qDebug()<<"拷贝"<<CData::app_info[i].name<< CData::app_info[i].time<<"id"<<CData::app_info[i].meet_id;
    }
    qDebug()<<"一共"<<resp.count;






}
