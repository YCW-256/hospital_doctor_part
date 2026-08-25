#include "CircularAvatar.h"
#include <QPainter>
#include <QPainterPath>

CircularAvatar::CircularAvatar(QWidget *parent)
    : QLabel(parent)
{
    // 不启用 QLabel 自带的缩放，由我们自己绘制
    setScaledContents(false);
    // 默认固定尺寸，方便演示（可自由调整）
    setFixedSize(100, 100);
}

void CircularAvatar::setAvatar(const QPixmap &pixmap)
{
    m_originalPixmap = pixmap;
    update(); // 触发重绘
}

void CircularAvatar::setAvatar(const QString &filePath)
{
    setAvatar(QPixmap(filePath));
}

void CircularAvatar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // 如果没有图片，调用基类绘制（可显示默认背景或空）
    if (m_originalPixmap.isNull()) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 计算控件中心的最大内切圆形区域
    QRect rect = this->rect();
    int side = qMin(rect.width(), rect.height());
    QRect circleRect(0, 0, side, side);
    circleRect.moveCenter(rect.center());

    // 构建圆形裁剪路径
    QPainterPath path;
    path.addEllipse(circleRect);
    painter.setClipPath(path);

    // 将原图等比缩放并裁剪到控件大小（保持比例，多余部分裁掉）
    QPixmap scaledPixmap = m_originalPixmap.scaled(
        rect.size(),
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
        );

    // 在控件上绘制图片（缩放后的图片左上角与控件对齐，超出圆形部分被裁剪）
    painter.drawPixmap(rect.topLeft(), scaledPixmap);
}

void CircularAvatar::mousePressEvent(QMouseEvent *event)
{
    // 可调用基类以确保原有行为（如果有）
    QLabel::mousePressEvent(event);

    QRect rect = this->rect();
    int side = qMin(rect.width(), rect.height());
    QRect circleRect(0, 0, side, side);
    circleRect.moveCenter(rect.center());

    QPainterPath path;
    path.addEllipse(circleRect);

    if (path.contains(event->pos())) {
        qDebug() << "点击头像即可";
        emit AvatarClicked();
    }


}