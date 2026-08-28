#ifndef CDATA_H
#define CDATA_H
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <QDebug>
#include <vector>
#include "protecol.h"
using namespace std;

typedef struct{
    QString name;
    QString time;
    int state;
}APP_INFO;
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
    //-----------------预约信息---------------------

    static vector<APP_INFO> app_info;
    //-----------------预约信息---------------------

    //科室医生信息
    static vector<DOCCTOR_INFO>selece_department_info;

};

#endif // CDATA_H
