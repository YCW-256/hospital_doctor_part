#ifndef PROTECOL_H
#define PROTECOL_H

#include <stdbool.h>

//--------------------------通信协议

enum SERVICE_TYPE {

    DOCTOR_LOGIN,
    DOCTOR_APP_INFO,
    SELECT_DOCTOR,
    REPIX_GUARD,//修改值班表
    GET_GUARD,

    //此处患者
    PATIENT_RESIGN,
    PATIENT_LOGIN,
    PATIENT_GET_DOCTOR_INFO,
    PATIENT_APPOINTMENT,

    DOCTOR_SET_RECORD,

    GET_MEDICAL_RECORD,        //病历【第一套·列表】按 do
    GET_MEDICAL_RECORD_DETAIL, //病历【第二套·
    IMG_UPLOAD,                 //舌苔图片上传
    GET_TONGUE_IMG,             //舌苔图片：请求带 医生 id + 患者 id + 日期；回包为若干 IMG_T 分片
};

typedef struct {
    SERVICE_TYPE type;
    int is_fragment;
    int msg_sn;
    int frag_index;
    int frag_total;
    int len;//数据包长度
}HEAD;
//医生登录
typedef struct {
    int login_style;
    char account[20];
    char pwd[20];


}DOCTOR_LOGIN_REQ;

typedef struct {
    int id;
    int result;
    char pwd[20];
    int role;

}DOCTOR_LOGIN_RESP;
//医生获得预约
typedef struct {
    int style;// 日 周 月
    int id;
}DOCTOR_APP_REQ;

typedef struct {

    char time[10][15];
    char DoctorName[10][15];
    char PatientName[10][15];
    int state[10];
    int id;
    int count;
    int meet_id[10];
    int patient_id[10];

}DOCTOR_APP_RESP;

typedef struct {
    int id;
    char department[15];
}DOCTOR_INFO_REQ;

typedef struct {
    int id;
    char name[20];
}DOCCTOR_INFO;

#pragma pack(push, 1)

//建立
typedef struct GUARD_REPIX_T {
    int id;
    char name[15];//值班人
    char date[20];//日期;
    int time;//时间
    bool isfree;//是否应用
    char depart[20];//科室
}GUARD_REPIX_T;
//获得
typedef struct {
    struct GUARD_REPIX_T guards[3][7];
}GET_GUARD_RESP;
#pragma pack(pop)

typedef struct {
    int id;
    char department[20];
    char start_day[20];
}GET_GUARD_REQ;

typedef struct {
    int meet_id;
    int doctor_id;
    int patient_id;
    char diagnosis[200];
    char treat_plan[200];
}SET_RECORD_REQ;

//------------------------------患者

typedef struct {
    char name[20];
    char card[20];
    char phone[15];
    char pwd[20];
}PATIENT_RESIGN_REQ;
typedef struct {
    int flag;
}PATIENT_RESIGN_RESP;


typedef struct {
    int type;
    char account[20];
    char pwd[20];
}PATIENT_LOGIN_REQ;


typedef struct {
    int result;
    int id;
    char name[20];

}PATIENT_LOGIN_RESP;

typedef struct {
    int id;
}PATIENT_GET_DOCTOR_REQ;

typedef struct {
    int id;
    char name[25];
    int time;
    char department[15];
}patient_doctor_infoo;

typedef struct {
    int patient_id;
    int doctor_id;
    int ob_time;
}PATIENT_APPOINTMENT_REQ;

typedef struct {
    int state;
}PATIENT_APPOINTMENT_RESP;











typedef struct {
    int id;
    bool isClass[12];

}WARING_REQ;

typedef struct {
    int id;
    int wet;
    int temperature;
}ENVIR_REQ;

typedef struct {
    int id;
    int wet;
    int temperature;
    bool isClass[12];
}ENVIR_RESP;
typedef struct {
    int id;
    int temperature;
}GETENVIR_REQ;

//------------------------------病历
typedef struct {
    int  doctor_id;          // 查询主键：接诊医生 id（取 CData::m_id）→ medical_record.doctor_id
    int  state;              // 病历状态过滤：0 正常 1 作废 -1 不限
    char patient_name[20];   // 患者姓名（模糊匹配），空 = 不限
    char date_begin[20];     // 就诊日期起 yyyy-MM-dd，空 = 不限（比 record_time 的日期部分）
    char date_end[20];       // 就诊日期止 yyyy-MM-dd，空 = 不限
}MEDICAL_RECORD_REQ;         // sizeof == 68（字段全是 4 的倍数，无隐藏填充）

// 列表项：只放“查询/选中”要用的字段，界面列直接照这个显示
typedef struct {
    int  record_id;          // 病历编号主键 —— 第二套详情请求带的就是它
    int  patient_id;         // 患者编号（界面要显示）
    int  state;              // 0 正常 1 作废
    char patient_name[20];   // 患者姓名（服务端 join 患者表）
    char record_time[20];    // 病历记录时间 yyyy-MM-dd HH:mm:ss
    char main_symptom[60];   // 主要症状摘要（列表列；完整内容在详情包里）
}MEDICAL_RECORD_LIST_ITEM;   // sizeof == 112

// 响应：total = 符合条件总条数（供界面“共 N 条”），count = 本包实际条数（<= MAX_ITEMS）
#define MEDICAL_RECORD_MAX_ITEMS 20
typedef struct {
    int total;
    int count;
    MEDICAL_RECORD_LIST_ITEM items[MEDICAL_RECORD_MAX_ITEMS];
}GET_MEDICAL_RECORD_RESP;    // sizeof == 2248，加 HEAD(24) 共 2272 < 4096

// ---- 第二套：详情 ----
typedef struct {
    int record_id;           // 第一套列表里选中那条的病历编号
    int doctor_id;           // 医生 id（服务端据此校验：只能取自己名下的病历）
}MEDICAL_RECORD_DETAIL_REQ;  // sizeof == 8

typedef struct {
    int  record_id;          // 病历编号主键
    int  patient_id;         // 患者编号
    int  doctor_id;          // 接诊医生 id
    int  state;              // 0 正常 1 作废
    char patient_name[20];   // 患者姓名（join 患者表）
    char patient_sex[4];     // 性别（join 患者表；服务端暂不提供就留空，界面显示空）
    int  patient_age;        // 年龄（join 患者表；同上）
    char record_time[20];    // 病历记录时间 yyyy-MM-dd HH:mm:ss
    char doctor_name[20];    // 接诊医生姓名（join 医生表）
    char main_symptom[100];  // 主要症状
    char diagnosis[200];     // 诊断结果
    char treat_plan[200];    // 治疗方案
}MEDICAL_RECORD_DETAIL_RESP;
#pragma pack(push, 1)
typedef struct {
    int index;
    int total;
    int width;
    int height;
    char img_data[8192];
    char file_name[100];
    int id;
}IMG_T;
#pragma pack(pop)

// ------------------------------舌苔图片
// 请求：医生 id + 患者 id + 就诊日期（年月日）。服务端按这三者找该次就诊的舌苔图片。
// 回包：**同一 type(GET_TONGUE_IMG) 的若干 IMG_T 分片**（一包 HEAD+IMG_T = 8336B，装不下整张图，
// 服务端按 8192B 一片切、index 从 0 递增、frag_total=总片数；找不到图时服务端不回包）。
// 本端接收见 Task/gettongueimgtask（跨包攒齐后拼 QImage 存 CData::tongue_image）。
typedef struct {
    int  doctor_id;       // 接诊医生 id（取 CData::m_id / MeetRecord::doctor_id）
    int  patient_id;      // 患者 id（DOCTOR_APP_RESP.patient_id）
    char date[20];        // 就诊日期 yyyy-MM-dd（从预约时间串里取日期部分）
}GET_TONGUE_IMG_REQ;      // sizeof == 28（字段全是 4 的倍数，无隐藏填充）

#endif // PROTECOL_H
