#ifndef WORKSTATWIDGET_H
#define WORKSTATWIDGET_H

#include <QWidget>
#include <QVector>

class QLabel;
class QFrame;

// 普通医生端“工作统计”页（左栏导航“工作统计”进入）。
// 顶部四个指标卡：今日访问量 / 本月访问量 / 本月出勤天数 / 加班次数。
// 数据为预留占位：CData::is_check==true 随机填充，false 显示 “--”；目前仅作装饰，真实统计待接。
class WorkStatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorkStatWidget(QWidget *parent = nullptr);

private:
    void buildUi();
    void seed(); // 按 CData::is_check 决定是否填充预留数据

    QFrame *makeStatCard(const QString &title, QLabel **valueOut);

    QVector<QLabel *> m_valueLabels; // 顺序：今日访问量/本月访问量/本月出勤天数/加班次数
};

#endif // WORKSTATWIDGET_H
