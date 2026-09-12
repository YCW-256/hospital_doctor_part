#include "gettongueimgtask.h"
#include "../MyTcp/protecol.h"
#include "../MyTcp/cdata.h"
#include <QImage>

// char[] 线上字段不一定带结束符：按“遇 \0 或写满”取有效长度再转 QString，避免越界读
static QString toStr(const char *buf, int maxLen)
{
    int n = 0;
    while(n < maxLen && buf[n] != '\0')
        ++n;
    return QString::fromUtf8(buf, n);
}

// 分片数上限（防脏数据把内存撑爆）：一片 8192B，8192 片 = 64MB，
// 足够 4600×4600 的 RGB 图，正常舌苔照（640×640 = 150 片）离得很远。
static const int kMaxFrags = 8192;

// ---- “正在收的那张图”的攒包状态 ----
// 一个分片被 new 一次 Task，对象用完就 delete，所以跨包状态只能放 static。
// 这些量只在 SocketLink 的 worker 线程里读写（recv_data 单线程跑），不需要加锁。
// 思路照搬服务端 FileTask：**按包的 index 把这一片拷到对应位置**（不是顺序 append），
// 这样就算中间某一片丢了/乱了，后面到的分片也能各就各位，只差缺的那一片。
static QByteArray    s_pixels;      // 预分配 total * 8192，第 i 片写在 i*8192 处
static std::vector<char> s_got;     // s_got[i] = 第 i 片已收到（防重复计数）
static int           s_count  = 0;  // 已收到几片
static int           s_total  = 0;  // 本张图总片数（IMG_T.total）
static int           s_width  = 0;
static int           s_height = 0;
static int           s_patientId = 0;
static QString       s_fileName;

static void resetAssembly()
{
    s_pixels.clear();
    s_got.clear();
    s_count = 0;
    s_total = 0;
    s_width = 0;
    s_height = 0;
    s_patientId = 0;
    s_fileName.clear();
}

GetTongueImgTask::GetTongueImgTask(QObject *parent)
    : BusinessTask{parent}
{

}

GetTongueImgTask::GetTongueImgTask(int len, QByteArray &data, QObject *parent)
    :BusinessTask(len,data,parent)
{

}

void GetTongueImgTask::execute()
{
    // 防护：只按实际收到的字节数拷，服务端多回/少回都不越界
    IMG_T pkt;
    memset(&pkt, 0, sizeof(pkt));
    int cp = this->len < static_cast<int>(sizeof(pkt)) ? this->len : static_cast<int>(sizeof(pkt));
    if (cp <= 0)
        return;
    memcpy(&pkt, this->data.constData(), cp);

    // 分片编号自检：服务端按 index 从 0 递增发，乱序/越界的直接丢掉
    if (pkt.total <= 0 || pkt.total > kMaxFrags || pkt.index < 0 || pkt.index >= pkt.total) {
        qDebug() << "舌苔图片分片编号异常，丢弃: index" << pkt.index << "total" << pkt.total;
        return;
    }

    // 第 0 片（或 total 变了 = 服务端换了张图）→ 重建缓冲区，元数据取本片带的
    if (pkt.index == 0 || s_total != pkt.total) {
        resetAssembly();
        s_total     = pkt.total;
        s_width     = pkt.width;
        s_height    = pkt.height;
        s_patientId = pkt.id;
        s_fileName  = toStr(pkt.file_name, sizeof(pkt.file_name));
        s_pixels.resize(static_cast<int>(sizeof(pkt.img_data)) * s_total);
        s_got.assign(s_total, 0);
    }

    if (s_got[static_cast<size_t>(pkt.index)])
        return;   // 重复片，忽略（别把 s_count 算重了）

    // ---- 按 index 拷到对应位置 ----
    memcpy(s_pixels.data() + static_cast<qsizetype>(pkt.index) * static_cast<qsizetype>(sizeof(pkt.img_data)),
           pkt.img_data, sizeof(pkt.img_data));
    s_got[static_cast<size_t>(pkt.index)] = 1;
    ++s_count;

    qDebug() << "收到舌苔图片分片" << (pkt.index + 1) << "/" << s_total
             << "（已收" << s_count << "片，共" << s_pixels.size() << "字节）"
             << "尺寸" << pkt.width << "x" << pkt.height;

    if (s_count < s_total) {
        // 最后一片都到了还没凑齐 → 中间缺片（服务端发残/丢包），这张拼不出来，放弃本次。
        // 医生重新点“获得图片”，服务端会从 index 0 重发一遍。
        if (pkt.index == s_total - 1) {
            qDebug() << "舌苔图片缺片，放弃本次：已收" << s_count << "/" << s_total << "片";
            resetAssembly();
        }
        return;
    }

    // ---- 收齐了：按 宽×高 把原始像素拼成 QImage ----
    const int w = s_width;
    const int h = s_height;
    if (w <= 0 || h <= 0 || w > 10000 || h > 10000) {
        qDebug() << "舌苔图片尺寸异常，丢弃: " << w << "x" << h;
        resetAssembly();
        return;
    }

    // 末片 img_data 后面是服务端 memset 补的 0，长度按 宽×高 截，补的 0 不参与拼图
    const qint64 needRgb  = static_cast<qint64>(w) * h * 3;   // 服务端 read_ppm 出来的是 RGB 裸数据
    const qint64 needGray = static_cast<qint64>(w) * h;       // 万一是灰度 PGM，兜底按 8 位灰度拼
    QImage img;
    if (s_pixels.size() >= needRgb) {
        img = QImage(reinterpret_cast<const uchar *>(s_pixels.constData()),
                     w, h, w * 3, QImage::Format_RGB888).copy();
    }
    else if (s_pixels.size() >= needGray) {
        img = QImage(reinterpret_cast<const uchar *>(s_pixels.constData()),
                     w, h, w, QImage::Format_Grayscale8).copy();
    }

    if (img.isNull()) {
        qDebug() << "舌苔图片拼装失败：像素字节数不够 " << s_pixels.size()
                 << "（RGB 需要" << needRgb << "，灰度需要" << needGray << "）";
        resetAssembly();
        return;
    }

    CData::tongue_image            = img;
    CData::tongue_image_patient_id = s_patientId;
    CData::tongue_image_file       = s_fileName;
    is_complete = true;

    qDebug() << "========== 收到舌苔图片 ==========";
    qDebug() << "文件名:" << s_fileName << " 患者编号:" << s_patientId;
    qDebug() << "尺寸:" << img.width() << "x" << img.height()
             << " 格式:" << static_cast<int>(img.format())
             << " 分片数:" << s_total << "/" << s_count;
    qDebug() << "==================================";

    resetAssembly();
}
