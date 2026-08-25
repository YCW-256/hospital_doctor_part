#ifndef CDATA_H
#define CDATA_H
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <QDebug>
using namespace std;
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
};

#endif // CDATA_H
