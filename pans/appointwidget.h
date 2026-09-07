#ifndef APPOINTWIDGET_H
#define APPOINTWIDGET_H
#include <QByteArray>
#include <QString>
#include <QWidget>

// pans/childs/doctor/appointdetailwidget.h 里定义的接诊记录快照（sendRecord 参数用）
struct MeetRecord;

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

signals:
    void to_get_meet(const QByteArray data,int len);
private:
    void init_connect();

    Ui::AppointWidget *ui;

    void getAppInfo(int idx);
    // 点中某张预约卡片 → 弹出“接诊详情”弹窗（idx 为 CData::app_info 下标）
    void openMeetDetail(int idx);
    // 弹窗里点“完成”→ 打包 DOCTOR_SET_RECORD 发服务器（诊断/处方）
    void sendRecord(const MeetRecord &rec, const QString &diagnosis, const QString &treatPlan);


};

#endif // APPOINTWIDGET_H
