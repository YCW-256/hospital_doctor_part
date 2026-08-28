#include "myutils.h"

MyUtils::MyUtils() {


}

void MyUtils::setIcons(QToolButton* button, const QString& text, const QString& img_path)
{
    if (!button) return;
    button->setText(text);
    button->setIcon(QIcon(img_path));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIconSize(QSize(100, 100));

    button->setStyleSheet(
        "QToolButton {"
        "    background: transparent;"
        "    border: none;"
        "}"
        "QToolButton:hover {"
        "    background: rgba(240, 240, 240, 80);"   // 悬停时半透明白色背景（30/255）
        "    border: 2px solid rgba(255,255,255,80);" // 可选：增加微弱边框
        "}"
        "QToolButton:pressed {"
        "    background: rgba(255, 255, 255, 60);"   // 按下时更明显
        "}"
        "QToolButton::text {"
        "    margin-top: -40px;"
        "}"
        );
}

void MyUtils::setBack(QWidget *widget, QString color1, QString color2)
{
    if (!widget) return;

    QString styleSheet = QString(
                             "QWidget {"
                             "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                             "                                 stop:0 %1, stop:1 %2);"
                             "}"
                             ).arg(color1, color2);

    widget->setStyleSheet(styleSheet);
}

void MyUtils::setLabel(QLabel *label,int font_size,bool is_bold)
{
    if (!label) return;
    if (font_size <= 0) font_size = 20;
    // 1. 背景设置为透明
    label->setAttribute(Qt::WA_TranslucentBackground);
    label->setAutoFillBackground(false);

    // 2. 设置文字颜色 #8AAEDE
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, QColor("#8AAEDE"));
    label->setPalette(pal);

    // 3. 设置字体
    QFont font = label->font();
    font.setFamily("Segoe UI"); // 或者 "Arial", "Verdana" 等圆润字体
    font.setPointSize(font_size);//字体大小
    font.setBold(is_bold);    // 图片看起来比较粗
    // 设置字间距，让看起来更开阔
    font.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    label->setFont(font);
}

void MyUtils::setLabelImg(QLabel *label, const QString &img_path, int size)
{
    if (!label) return;

    // 1. 背景设置为透明
    label->setAttribute(Qt::WA_TranslucentBackground);
    label->setAutoFillBackground(false);
    label->setStyleSheet("background-color: transparent;");

    // 2. 加载图片
    QPixmap pixmap(img_path);
    if (pixmap.isNull()) {
        return; // 图片加载失败，保持空
    }

    // 3. 缩放图片 (默认为正方形，忽略原始比例)
    pixmap = pixmap.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 4. 设置到 Label
    label->setPixmap(pixmap);

    // 5. 固定 Label 大小
    label->setFixedSize(size, size);
    label->setAlignment(Qt::AlignCenter);
}

void MyUtils::weekAndDate(QLabel *labels[], QDate date)
{
    if (!labels) return;

    // 计算 date 所在周的周一（Qt: Monday=1, Sunday=7）
    QDate monday = date.addDays(- (date.dayOfWeek() - 1));

    for (int i = 0; i < 7; ++i) {
        QDate day = monday.addDays(i);
        QString dateStr = QString("%1月%2日").arg(day.month()).arg(day.day());
        labels[i]->setText(dateStr);
    }
}

void MyUtils::weekAndDate(QLabel *labels[], GUARD_REPIX_T info[][7], QDate date)
{
    if (!labels) return;

    // 计算 date 所在周的周一（Qt: Monday=1, Sunday=7）
    QDate monday = date.addDays(- (date.dayOfWeek() - 1));

    for (int i = 0; i < 7; ++i) {
        QDate day = monday.addDays(i);
        QString dateStr = QString("%1月%2日").arg(day.month()).arg(day.day());
        labels[i]->setText(dateStr);

        // 将日期以 "yyyy-MM-dd" 格式存入 info 的 time 字段中
        QString isoDate = day.toString("yyyy-MM-dd");   // 例如 "2026-08-28"
        QByteArray ba = isoDate.toLocal8Bit();
        const char* cstr = ba.constData();

        for (int j = 0; j < 3; ++j) {
            // 假设 time 数组大小足够（至少 11 字节，因为 "yyyy-MM-dd" 长度为 10 + 结尾符）
            strncpy_s(info[j][i].date, cstr, sizeof(info[j][i].date) - 1);
            // 确保字符串以 '\0' 结尾
            info[j][i].date[sizeof(info[j][i].date) - 1] = '\0';
        }
    }
}

QString MyUtils::getThisWeekMondayStr(const QDate &date)
{
    int day = date.dayOfWeek();
    QDate monday = date.addDays( -(day - 1) );
    return monday.toString("yyyy-MM-dd");
}