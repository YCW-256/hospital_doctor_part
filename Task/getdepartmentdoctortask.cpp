#include "getdepartmentdoctortask.h"

GetDepartmentDoctorTask::GetDepartmentDoctorTask(QObject *parent)
    : BusinessTask{parent}
{}

GetDepartmentDoctorTask::GetDepartmentDoctorTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask(len,data,parent)
{

}

void GetDepartmentDoctorTask::execute()
{
    const char* data = this->data.constData();   // 获取数据指针
    DOCCTOR_INFO doc;
    CData::selece_department_info.clear();
    int cur_size = 0;
    while (cur_size < len) {
        memcpy((char*)&doc, data + cur_size, sizeof(DOCCTOR_INFO));
        CData::selece_department_info.push_back(doc);
        cur_size += sizeof(DOCCTOR_INFO);
        qDebug() << doc.id << doc.name;
        qDebug() << cur_size;
    }

    for (int i = 0; i < CData::selece_department_info.size(); ++i) {
        qDebug() << "id:" << CData::selece_department_info[i].id
                 << "name:" << CData::selece_department_info[i].name;
    }

}
