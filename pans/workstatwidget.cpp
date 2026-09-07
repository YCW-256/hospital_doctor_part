#include "workstatwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QRandomGenerator>
#include <QStringList>
#include "../MyTcp/cdata.h"

WorkStatWidget::WorkStatWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    seed();
}

QFrame *WorkStatWidget::makeStatCard(const QString &title, QLabel **valueOut)
{
    QFrame *card = new QFrame(this);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(
        "QFrame { background-color:#FFFFFF; border:1px solid #D9E7F5; border-radius:12px; }");
    card->setMinimumHeight(150);

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(14, 18, 14, 18);
    lay->setSpacing(8);

    *valueOut = new QLabel(this);
    (*valueOut)->setAlignment(Qt::AlignCenter);
    (*valueOut)->setStyleSheet(
        "QLabel { background: transparent; border: none;"
        " color:#2F80ED; font-size:32px; font-weight:bold; }");
    lay->addWidget(*valueOut);

    QLabel *cap = new QLabel(title, this);
    cap->setAlignment(Qt::AlignCenter);
    cap->setStyleSheet(
        "QLabel { background: transparent; border: none; color:#6B7B8C; font-size:14px; }");
    lay->addWidget(cap);

    lay->addStretch();
    return card;
}

void WorkStatWidget::buildUi()
{
    this->setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("WorkStatWidget { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                  " stop:0 #E8F2F6, stop:1 #FFFFFF); }");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    QLabel *title = new QLabel(QStringLiteral("工作统计"), this);
    title->setStyleSheet("background: transparent; border:none; font-size:20px;"
                         " font-weight:bold; color:#1E70BF;");
    root->addWidget(title);

    QLabel *sub = new QLabel(QStringLiteral("今日 / 本月概况（预置占位数据，仅装饰）"), this);
    sub->setStyleSheet("background: transparent; border:none; font-size:13px; color:#8A9AB0;");
    root->addWidget(sub);

    // 一行四张指标卡
    QHBoxLayout *cards = new QHBoxLayout;
    cards->setSpacing(16);
    const QStringList titles = {QStringLiteral("今日访问量"), QStringLiteral("本月访问量"),
                                QStringLiteral("本月出勤天数"), QStringLiteral("加班次数")};
    for (const QString &t : titles) {
        QLabel *val = nullptr;
        QFrame *card = makeStatCard(t, &val);
        m_valueLabels.append(val);
        cards->addWidget(card, 1);
    }
    root->addLayout(cards, 1);

    root->addStretch();
}

void WorkStatWidget::seed()
{
    // 预留占位数据：CData::is_check==true 随机填，false 显示 “--” 等真实数据
    QStringList texts;
    if (CData::is_check) {
        QRandomGenerator *r = QRandomGenerator::global();
        texts << QString::number(r->bounded(5, 60))    // 今日访问量
              << QString::number(r->bounded(150, 420)) // 本月访问量
              << QString::number(r->bounded(15, 26))   // 本月出勤天数
              << QString::number(r->bounded(0, 10));   // 加班次数
    } else {
        texts << QStringLiteral("--") << QStringLiteral("--")
              << QStringLiteral("--") << QStringLiteral("--");
    }

    for (int i = 0; i < m_valueLabels.size() && i < texts.size(); ++i)
        m_valueLabels[i]->setText(texts.at(i));
}
