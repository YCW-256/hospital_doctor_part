#include "chatbom.h"
#include "ui_chatbom.h"
#include <QStyle>
ChatBom::ChatBom(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChatBom)
{
    ui->setupUi(this);
}
int MAX_LINE_WIDTH = 460;
int MIN_LINE_WIDTH = 50;
int TOTAL_MARGINS = 6;

ChatBom::ChatBom(QWidget *parent, int pos)
    : QWidget(parent)
{
    this->setMaximumHeight(10000);
    //1  创建头像
    m_avatar = new QLabel(this);
    m_avatar->setFixedSize(40, 40);
    m_avatar->setStyleSheet("border: 1px solid gray; border-radius: 4px;");
    m_avatar->setScaledContents(true);

    //2  创建文本编辑框（只读，显示用）
    m_textEdit = new QTextEdit(this);
    //m_textEdit->setReadOnly(true);m_
    m_textEdit->setWordWrapMode(QTextOption::WrapAnywhere);
    m_textEdit->setLineWrapMode(QTextEdit::WidgetWidth);

    //3  为文本编辑框创建统一格式
    this->setContentsMargins(0,0,0,0);

    //          垂直滚动条关闭
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //          水平滚动条关闭
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_textEdit->document()->setDocumentMargin(0);
    m_textEdit->setMaximumHeight(100000);
    QString qss;
    if(pos==LAYOUT_STYLE::LEFT){
        setAvatar(":/icons/avert2.jpg");
        m_textEdit->setProperty("group", "chat_bom_left");
        qss = R"(
            QTextEdit[group="chat_bom_left"] {
            background-color: #F0F2F5;
            color: #1F1F1F;
            font-size: 12pt;
            border: none;
            border-radius:5px;
            padding-top: 8px;
            padding-bottom: 4px;
            margin: 0px;
            }
        )";
    }
    else{
        setAvatar(":/icons/avert1.jpg");
        m_textEdit->setProperty("group", "chat_bom_right");
        qss = R"(
            QTextEdit[group="chat_bom_right"] {
            background-color: #3088E8;
            color: white;
            font-size: 12pt;
            border: none;
            border-radius:5px;
            padding-top: 8px;
            padding-bottom: 4px;
            margin: 0px;
            }
        )";
    }
    m_textEdit->style()->unpolish(m_textEdit);
    m_textEdit->style()->polish(m_textEdit);
    m_textEdit->setStyleSheet(qss);
    m_textEdit->ensurePolished();

    m_textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);




    // 主水平布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(TOTAL_MARGINS, TOTAL_MARGINS, TOTAL_MARGINS, TOTAL_MARGINS);

    mainLayout->setSpacing(childs_spa);
    if (pos == LEFT) {
        // [头像] [文本] [弹簧]
        mainLayout->addWidget(m_avatar,1);
        mainLayout->addWidget(m_textEdit,1);
        mainLayout->addStretch(1);
    } else { // RIGHT
        // [弹簧] [文本] [头像]
        mainLayout->addStretch(1);
        mainLayout->addWidget(m_textEdit,10);
        mainLayout->addWidget(m_avatar,1);
    }
    // ---- 关键：让头像（和文本）在垂直方向上靠顶部对齐 ----
    mainLayout->setAlignment(m_avatar, Qt::AlignTop);
    mainLayout->setAlignment(m_textEdit, Qt::AlignTop);
}

ChatBom::~ChatBom()
{
    delete ui;
}

void ChatBom::setMessage(const QString &text)
{

    //设置字体，并获得文本的单行宽
    m_textEdit->setText(text);
    QFontMetrics fm(m_textEdit->font());
    int text_width=fm.horizontalAdvance(text);

    //执行自适应策略
    int target_width;  //1                            2                    3                   4
    target_width=(text_width+TOTAL_MARGINS*2+40+pre+childs_spa*3) < (MAX_LINE_WIDTH)?(text_width):(MAX_LINE_WIDTH-TOTAL_MARGINS*2-40-pre-childs_spa*3);

    //qDebug()<<target_width;

    m_textEdit->setFixedWidth(target_width);
    m_textEdit->document()->setTextWidth(m_textEdit->width());
    int idealHeight = int(m_textEdit->document()->size().height()+TOTAL_MARGINS*2+12);
    m_textEdit->setFixedHeight(idealHeight);
    this->setFixedHeight(idealHeight);
    qDebug()<<"内部设置高"<<idealHeight<<"内部确定高"<<m_textEdit->height();


    // qDebug()<<"signle_line"<<text_width;
    // qDebug()<<"width "<<this->m_textEdit->width();
    // qDebug()<<"height "<<this->m_textEdit->height();


}

void ChatBom::setAvatar(QString img_path)
{
    QPixmap pixmap(img_path);
    if (pixmap.isNull()) return;
    QSize labelSize = m_avatar->size();
    if (labelSize.isEmpty()) {
        // 如果label还没布局好，可以设置固定尺寸或使用最大尺寸
        // 简单起见，先设置缩放内容为true或者直接设置原始图并拉伸
        m_avatar->setPixmap(pixmap);
        m_avatar->setScaledContents(true);
        return;
    }
    QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_avatar->setPixmap(scaledPixmap);
    m_avatar->setAlignment(Qt::AlignCenter); // 多余部分裁掉（效果由裁剪区域实现，但label不会裁剪，
}

void ChatBom::setTextWidth(int width)
{
   m_textEdit->setFixedWidth(width);
}

void ChatBom::setTextHeight(int height)
{
    m_textEdit->setFixedHeight(height);
}

QString ChatBom::getText()
{
    return this->m_textEdit->toPlainText();
}