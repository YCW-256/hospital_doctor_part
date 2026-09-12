#ifndef CDATA_H
#define CDATA_H
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <QDebug>
#include <QHash>
#include <vector>
#include "protecol.h"
#include <QWidget>
#include <QImage>
using namespace std;

typedef struct{
    QString name;        // 患者姓名（DOCTOR_APP_RESP.PatientName）
    QString time;        // 就诊时间/时段（DOCTOR_APP_RESP.time）
    int state;
    int meet_id;
    QString doctorName;  // 接诊医生名（DOCTOR_APP_RESP.DoctorName，接诊详情弹窗用）
    int patient_id;
}APP_INFO;

// 病历【第一套·列表】的一条（MEDICAL_RECORD_LIST_ITEM 的 QString 版）
typedef struct{
    int record_id;         // 病历编号主键 —— 选中后第二套详情请求带的就是它
    int patient_id;        // 患者编号
    int state;             // 0 正常 1 作废
    QString patient_name;  // 患者姓名
    QString record_time;   // 病历记录时间 yyyy-MM-dd HH:mm:ss
    QString main_symptom;  // 主要症状摘要
}MEDICAL_RECORD_INFO;

// 病历【第二套·详情】的一条（MEDICAL_RECORD_DETAIL_RESP 的 QString 版）
typedef struct{
    int record_id;
    int patient_id;
    int doctor_id;
    int state;
    QString patient_name;
    QString patient_sex;
    int patient_age;
    QString record_time;
    QString doctor_name;
    QString main_symptom;
    QString diagnosis;     // 诊断结果
    QString treat_plan;    // 治疗方案 / 治疗意见
}MEDICAL_RECORD_DETAIL_INFO;
class CData
{
public:
    CData();
    static vector<int>devices;
    static void init();
    static bool read_devices_id();
    static bool readJsonFile();
    //---变量
    static QString ip;
    static int port;
    static int current_width;
    static int current_height;
    static int m_id;
    static int m_role;
    // 工作统计是否使用预留(占位)假数据：true=各统计页用随机数填充展示；false=留空等真实数据。main 里赋值。
    static bool is_check;
    //-----------------预约信息---------------------

    static vector<APP_INFO> app_info;
    //-----------------预约信息---------------------

    //科室医生信息
    static vector<DOCCTOR_INFO>selece_department_info;

    //修改的值班信息
    static vector<GUARD_REPIX_T>repix_info;

    //导入卡片
    static GET_GUARD_RESP m_get_cards;

    //-----------------病历（两套协议，见 protecol.h）---------------------
    // 第一套：列表查询结果（每次回包整体覆盖）
    static vector<MEDICAL_RECORD_INFO> medical_record_list;
    static int medical_record_total;                        // 符合条件总条数（resp.total，供“共 N 条”）
    // 第二套：详情缓存，key = record_id（选中某条后按需拉取，来一条存一条）
    static QHash<int, MEDICAL_RECORD_DETAIL_INFO> medical_record_details;
    //-----------------病历（两套协议）---------------------

    //-----------------舌苔图片（GET_TONGUE_IMG 分片回包，见 Task/gettongueimgtask）---------------------
    // 整张图收齐后才写入（中途的分片不动这几项），界面靠 patient_id 核对是不是自己等的那张
    static QImage  tongue_image;             // 最近一次收齐的舌苔图片（isNull = 没有/没收全）
    static int     tongue_image_patient_id;  // 该图属于哪个患者（IMG_T.id）
    static QString tongue_image_file;        // 服务端给的文件名（调试用）
    //-----------------舌苔图片---------------------

    static QWidget* current_widget;
};

#endif // CDATA_H
