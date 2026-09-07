#ifndef MANAGERSTATWIDGET_H
#define MANAGERSTATWIDGET_H

#include <QWidget>
#include <QStringList>
#include <QVector>

class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;

// 自绘装饰用折线图控件（QPainter 手绘，不引入 QtCharts）。
// 给一组(横轴名, 值)画带渐变面积的折线；空数据显示“暂无数据”。
class TrendChartWidget : public QWidget
{
public:
    explicit TrendChartWidget(QWidget *parent = nullptr);

    void setSeries(const QStringList &xLabels, const QVector<int> &values); // labels 与 values 同长
    void clearSeries();                                                     // 清空并显示“暂无数据”

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList m_labels;
    QVector<int> m_values;
};

// 管理员工作统计页（左栏导航“工作统计”进入，pans/mannger/ 下）。
// 两个子视图（内部 QStackedWidget 切换）：
//   1) 访问量趋势（默认）：自绘折线图 + 顶部 日/周/月/年 分段按钮组切换刻度；
//   2) 医生工作量：右上“医生工作量”按钮切入，表格一行一位医生（医生|月出勤天数|月访问量|月加班次数）。
// 数据全部是预留占位：CData::is_check==true 时随机填充展示，false 时留空；目前仅作装饰，真实统计待接。
class ManagerStatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ManagerStatWidget(QWidget *parent = nullptr);

private slots:
    void onScaleChanged(int idx); // 日/周/月/年 切换
    void onToggleView();          // 访问量趋势 <-> 医生工作量

private:
    void buildUi();
    void seedTrendByScale();    // 按当前 m_scale 生成横轴/数值（is_check 时才填）
    void refreshTrend();        // 把当前数据刷到折线图
    void seedWorkload();        // 生成医生工作量表数据（is_check 时才填）

    int m_scale = 0; // 0=日 1=周 2=月 3=年

    QLabel         *m_titleLabel;
    QPushButton    *m_toggleBtn;  // “医生工作量 / 查看趋势” 切换按钮
    QStackedWidget *m_stack;
    QWidget        *m_chartPage;
    QWidget        *m_workloadPage;
    TrendChartWidget *m_chart;
    QTableWidget   *m_workTable;

    QStringList m_xLabels;   // 当前刻度的横轴名
    QVector<int> m_values;   // 当前刻度的访问量
};

#endif // MANAGERSTATWIDGET_H
