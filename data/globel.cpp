#include "globel.h"
int TOTAL_WIDTH = 1200;
int TOTAL_HEIGHT = 600;
string config_path="../configs/config.txt";
bool load_config(){
    ifstream ifs(config_path);
    if (!ifs.is_open())
    {
        std::cout << "读取配置失败" << std::endl;
        return false;
    }

    std::string line;
    // 逐行读取
    ifs>>TOTAL_WIDTH;
    ifs>>TOTAL_HEIGHT;
    qDebug()<<"TOTAL_WIDTH:"<<TOTAL_WIDTH<<" TOTAL_HEIGHT:"<<TOTAL_HEIGHT;
    ifs.close();
    return true;


};