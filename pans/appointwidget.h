#ifndef APPOINTWIDGET_H
#define APPOINTWIDGET_H
#include <QByteArray>
#include <QString>
#include <QWidget>

// pans/childs/doctor/appointdetailwidget.h 里定义的接诊记录快照（sendRecord 参数用）
struct MeetRecord;
// 当前打开的接诊详情弹窗（舌苔图片回包要转给它显示）
class AppointDetailWidget;

enum SEL_TIME{
    DAY,
    WEEK,
    MOUNTH
};

namespace Ui {
class AppointWidget;
}

class AppointWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AppointWidget(QWidget *parent = nullptr);
    ~AppointWidget();

    void flush();

    // 下行：SocketLink::get_tongue_img_success（整张舌苔图片已收齐、CData 已刷新）
    // 转给当前打开的接诊详情弹窗显示；弹窗没开就忽略（图留在 CData 里，下次开窗直接用）
    void flush_tongue_img();

signals:
    void to_get_meet(const QByteArray data,int len);
private:
    void init_connect();

    Ui::AppointWidget *ui;

    // 当前打开的接诊详情弹窗（栈对象，exec() 返回后必须立刻置空，别留悬空指针）
    AppointDetailWidget *m_detailDlg = nullptr;

    void getAppInfo(int idx);
    // 点中某张预约卡片 → 弹出“接诊详情”弹窗（idx 为 CData::app_info 下标）
    void openMeetDetail(int idx);
    // 弹窗里点“完成”→ 打包 DOCTOR_SET_RECORD 发服务器（诊断/处方）
    void sendRecord(const MeetRecord &rec, const QString &diagnosis, const QString &treatPlan);
    // 弹窗里点“获得图片”→ 打包 GET_TONGUE_IMG 发服务器（医生 id / 患者 id / 日期），回包为分片图片
    void sendGetTongueImg(const MeetRecord &rec, const QString &date);


};

#endif // APPOINTWIDGET_H
