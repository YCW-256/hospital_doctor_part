#ifndef SELBTN_H
#define SELBTN_H

#include <QWidget>
#include <QStringList>

class QButtonGroup;

class SelBtn : public QWidget
{
    Q_OBJECT
public:
    explicit SelBtn(QWidget *parent = nullptr);

    void setCurrentIndex(int index);
    int currentIndex() const;



signals:
    void currentIndexChanged(int index);

private:
    QButtonGroup *m_buttonGroup;
};

#endif // SELBTN_H