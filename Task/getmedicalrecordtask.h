#ifndef GETMEDICALRECORDTASK_H
#define GETMEDICALRECORDTASK_H

#include "businesstask.h"

// 病历【第一套·列表】：解析 GET_MEDICAL_RECORD_RESP，写入 CData::medical_record_list
// （每次回包整体覆盖，和 GetInfoTask 对 app_info 的做法一致）
class GetMedicalRecordTask : public BusinessTask
{
public:
    explicit GetMedicalRecordTask(QObject *parent = nullptr);

    GetMedicalRecordTask(int len,QByteArray &data ,QObject *parent);

    void execute();

};

#endif // GETMEDICALRECORDTASK_H
