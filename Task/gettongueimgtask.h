#ifndef GETTONGUEIMGTASK_H
#define GETTONGUEIMGTASK_H

#include "businesstask.h"

// 舌苔图片回包：服务端把一张图切成若干 IMG_T 分片发（一个分片的 img_data 只有 8192B，
// 而 HEAD+IMG_T = 8336B 远超单包 4096 的老缓冲上限，所以必须分片）。
// 本 Task 每收到一片被 new 一次（用完即 delete），分片靠 .cpp 里的 static 状态跨包累计：
// **按包的 index 直接拷到缓冲区里 index*8192 的位置**（照搬服务端 FileTask 那套），
// **收齐全部片**才拼成 QImage 写进 CData::tongue_image，并把 is_complete 置 true；
// 调用方（socketlink::recv_data）只在 is_complete 时才 emit get_tongue_img_success()。
// 按 index 落位的好处：中间丢了一片也不会把后面到的分片全废掉，只差那一片；
// 到最后一片都到齐了还没凑满就报一行日志放弃本次，医生重新点“获得图片”即从头重发。
class GetTongueImgTask : public BusinessTask
{
public:
    explicit GetTongueImgTask(QObject *parent = nullptr);

    GetTongueImgTask(int len,QByteArray &data ,QObject *parent);

    void execute();

    // 本次调用是否把整张图收齐并已写入 CData（中途的分片为 false）
    bool is_complete = false;
};

#endif // GETTONGUEIMGTASK_H
