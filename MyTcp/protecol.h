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

typedef struct{
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

#endif // PROTECOL_H
