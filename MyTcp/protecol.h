#ifndef PROTECOL_H
#define PROTECOL_H

//--------------------------通信协议

enum SERVICE_TYPE {

    DOCTOR_LOGIN,


};

typedef struct {
    SERVICE_TYPE type;
    int is_fragment;
    int msg_sn;
    int frag_index;
    int frag_total;
    int len;//数据包长度
}HEAD;
typedef struct {
    int login_style;
    char account[20];
    char pwd[20];

}DOCTOR_LOGIN_REQ;

typedef struct {
    int id;
    int result;
    char pwd[20];

}DOCTOR_LOGIN_RESP;




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
typedef struct{
    int id;
    int temperature;
}GETENVIR_REQ;

#endif // PROTECOL_H
