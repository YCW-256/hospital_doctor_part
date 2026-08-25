#ifndef GLOBEL_H
#define GLOBEL_H
#include <fstream>   // 文件流 ifstream / ofstream / fstream
#include <string>    // std::string
#include <iostream>
#include "QDebug"
using namespace std;


extern int TOTAL_WIDTH;
extern int TOTAL_HEIGHT;
extern string config_path;
bool load_config();


#endif // GLOBEL_H
