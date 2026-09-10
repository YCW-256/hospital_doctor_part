#include "getmedicalrecorddetailtask.h"
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"

// char[] 线上字段不一定带结束符：按“遇 \0 或写满”取有效长度再转 QString，避免越界读
static QString toStr(const char *buf, int maxLen)
{
    int n = 0;
    while(n < maxLen && buf[n] != '\0')
        ++n;
    return QString::fromUtf8(buf, n);
}

GetMedicalRecordDetailTask::GetMedicalRecordDetailTask(QObject *parent)
    : BusinessTask{parent}
{

}

GetMedicalRecordDetailTask::GetMedicalRecordDetailTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask(len,data,parent)
{

}

void GetMedicalRecordDetailTask::execute()
{
    MEDICAL_RECORD_DETAIL_RESP resp;
    memset(&resp,0,sizeof(resp));
    // 防护：只按实际收到的字节数拷贝，服务端多回/少回都不越界
    int cp = this->len < (int)sizeof(resp) ? this->len : (int)sizeof(resp);
    memcpy(&resp, this->data.constData(), cp);

    MEDICAL_RECORD_DETAIL_INFO info;
    info.record_id    = resp.record_id;
    info.patient_id   = resp.patient_id;
    info.doctor_id    = resp.doctor_id;
    info.state        = resp.state;
    info.patient_name = toStr(resp.patient_name, sizeof(resp.patient_name));
    info.patient_sex  = toStr(resp.patient_sex,  sizeof(resp.patient_sex));
    info.patient_age  = resp.patient_age;
    info.record_time  = toStr(resp.record_time,  sizeof(resp.record_time));
    info.doctor_name  = toStr(resp.doctor_name,  sizeof(resp.doctor_name));
    info.main_symptom = toStr(resp.main_symptom, sizeof(resp.main_symptom));
    info.diagnosis    = toStr(resp.diagnosis,    sizeof(resp.diagnosis));
    info.treat_plan   = toStr(resp.treat_plan,   sizeof(resp.treat_plan));

    // 按 record_id 存：界面选中哪条就取哪条，别的记录的详情不受影响
    CData::medical_record_details.insert(info.record_id, info);

    qDebug() << "========== 收到病历详情响应 ==========";
    qDebug() << "病历编号:" << info.record_id
             << "患者编号:" << info.patient_id
             << "医生id:" << info.doctor_id;
    qDebug() << "姓名:" << info.patient_name
             << "性别:" << info.patient_sex
             << "年龄:" << info.patient_age;
    qDebug() << "接诊医生:" << info.doctor_name
             << "记录时间:" << info.record_time
             << "状态:" << (info.state == 0 ? "正常" : "作废");
    qDebug() << "主要症状:" << info.main_symptom;
    qDebug() << "诊断:" << info.diagnosis;
    qDebug() << "治疗意见:" << info.treat_plan;
    qDebug() << "======================================";
}
