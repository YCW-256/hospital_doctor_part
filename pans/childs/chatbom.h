#ifndef CHATBOM_H
#define CHATBOM_H

#include <QWidget>
#include <QLabel>
#include <QTextEdit>

// 声明外部变量
// 定义全局变量，可以运行时修改
extern int MAX_LINE_WIDTH;
extern int MIN_LINE_WIDTH;
extern int TOTAL_MARGINS;

const int pre =40;//空出头像
const int childs_spa=10;// 头像与组件距离
namespace Ui {
class ChatBom;
}
enum LAYOUT_STYLE{
    LEFT,
    RIGHT
};
class ChatBom : public QWidget
{
    Q_OBJECT

public:
    explicit ChatBom(QWidget *parent = nullptr);
    explicit ChatBom(QWidget *parent = nullptr,int pos=LAYOUT_STYLE::LEFT);

    ~ChatBom();
    // 方便设置消息内容
    void setMessage(const QString &text);

    void setAvatar(QString path);

    void setTextWidth(int width);

    void setTextHeight(int height);

    QString getText();



private:
    Ui::ChatBom *ui;
    QLabel   *m_avatar;      // 头像

    QTextEdit *m_textEdit;   // 文本区域（支持多行）

};

#endif // CHATBOM_H
