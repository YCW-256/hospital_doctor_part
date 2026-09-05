#ifndef DOCTORORDER_H
#define DOCTORORDER_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QDate>
#include <QByteArray>
#include "../MyTcp/protecol.h"

class QLabel;
class QComboBox;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QTimer;
class DoctorSlotCard;

// 管理员排班页（医生 × 一周时段）
// 布局：左列 固定医生名（始终可见，纵向随表格滚动同步）
//       右侧 顶部固定时间栏(周一~周日日期，横向与表格联动) + 可滚动表格
// 表格：横轴=7 天，每天展开 上午/下午/晚上 三张无间距瓦片卡，天与天之间留间距。
// 默认从今天所在周的周一开始，上一周/下一周 翻周。
// 已接：选科室查医生(SELECT_DOCTOR)+本周排班(GET_GUARD)、点卡片指派/取消、
//      保存修改、批量复制排班(startCopyWeeks/定时器逐包)、智能排班(SmartScheduleDialog)。
class DoctorOrder : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorOrder(QWidget *parent = nullptr);
    ~DoctorOrder() override;

public slots:
    // SocketLink::get_doctor_info_success 到达后刷新医生名与表格
    void flush_doctor();

    // SocketLink::get_guard_info_success 到达后把排班显示到卡片
    void flush_table();

signals:
    // 与 OrderWidget::get_doctor_info 同型，窗口据此转发给 SocketLink::send_data
    void get_doctor_info(QByteArray data, int send_size);

    // GET_GUARD 排班查询
    void get_guard_info(QByteArray data, int send_size);

    // REPIX_GUARD 保存排班
    void save_guard_info(QByteArray data, int send_size);

private:
    void buildShell();      // 工具条 + 左列 + 时间栏 + 滚动区（只建一次）
    void rebuildContent();  // 依据 m_docNames 重建医生名列与表格行
    void refreshDates();    // 刷新时间栏日期（上一周/下一周用）

    void prevWeek();
    void nextWeek();
    void onDeptChanged(int index);

    void initInfo();        // 清空本地排班模型(m_info 全部空闲)
    void sendGuardRequest();// 发 GET_GUARD（发送前 sleep，目前缓解两包并发）
    void applyRoster();     // 用 m_info 重绘所有卡片（按行医生转置）
    void onCardToggled(int r, int d, int k); // 点击卡片：指派/取消 该行医生
    void dumpSchedule();    // 只打印修改过的项（每行医生名+id 及其被修改时段的当前状态）
    void saveInfo();        // 参照 orderWidget：把修改过的槽位打包发 REPIX_GUARD
    void openCopyDialog();  // 弹“批量复制排班”对话框，确认后交给 startCopyWeeks
    void startCopyWeeks(int weeks); // 把当前展示周整周打包成 weeks 个 REPIX_GUARD（每周一包）排队
    void sendNextCopyPack();        // 定时器节奏：逐包发 REPIX_GUARD，队列空则恢复按钮
    void openSmartDialog(); // 弹“智能排班”对话框（数据走快照，不再发服务器请求）
    void applySmartPlan(const GUARD_REPIX_T plan[3][7]); // 应用智能排班结果：按与旧排班的差异发 REPIX

    // 由 m_monday 算第 offset 天(0=周一)的日期文本，如 "周一\n8月31日"
    static QString dateText(const QDate &monday, int offset);

    QComboBox   *m_deptCombo;
    QPushButton *m_prevBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_copyBtn;    // 批量复制排班（弹窗入口）
    QPushButton *m_smartBtn;   // 智能排班（弹窗入口）
    QPushButton *m_nextBtn;
    QScrollArea *m_timeBar;   // 固定顶部时间栏
    QScrollArea *m_scroll;    // 右侧表格（横向+纵向滚动）
    QScrollArea *m_nameScroll;// 左侧固定医生名列（纵向随表格同步）
    QVBoxLayout *m_nameLayout;// 医生名纵向排列

    QDate                  m_monday;     // 当前展示周（周一）
    QVector<QString>       m_docNames;   // 当前科室医生名单（纵轴）
    QVector<QLabel*>       m_dateLabels; // 时间栏 7 个日期标签
    QVector<QLabel*>       m_nameLabels; // 每行医生名（放左列）
    QVector<int>           m_docIds;     // 与 m_docNames 平行的医生 id
    // 瓦片卡按 (医生r, 天d, 时段k) 顺序存放，行主序，供后续填数据用
    QVector<DoctorSlotCard*> m_cards;

    // 排班模型：m_info[k][d] 表示第 d 天第 k 时段(0上午/1下午/2晚上)的值班记录
    GUARD_REPIX_T          m_info[3][7];
    // 每位医生一个 21 位修改标记(7天*3段，位置=d*3+k，即 d 天 k 时段)：
    // 只要被点过就置 true，即使多次点击最终恢复原样也算“修改过”
    // （保存时只提交/打印这些修改过的位置）
    struct DocModified { bool flag[21]; };
    QVector<DocModified>   m_modified;

    // 批量复制排班：逐周待发 REPIX_GUARD 包队列 + 节奏定时器（代替连发包之间的 sleep）
    QVector<QByteArray>    m_copyQueue;
    QTimer                *m_copyTimer;
};

#endif // DOCTORORDER_H
