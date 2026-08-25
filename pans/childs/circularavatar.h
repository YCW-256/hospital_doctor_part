#ifndef CIRCULARAVATAR_H
#define CIRCULARAVATAR_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>
class CircularAvatar : public QLabel
{
    Q_OBJECT
public:
    explicit CircularAvatar(QWidget *parent = nullptr);

    // 设置头像（从QPixmap）
    void setAvatar(const QPixmap &pixmap);
    // 设置头像（从文件路径）
    void setAvatar(const QString &filePath);

protected:

    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;  // 新增：鼠标点击事件

private:
    QPixmap m_originalPixmap;   // 保存原始图片

signals:
    void AvatarClicked();
};

#endif // CIRCULARAVATAR_H