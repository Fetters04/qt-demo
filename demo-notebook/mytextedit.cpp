#include "mytextedit.h"

#include <QWheelEvent>
#include <QDebug>

MyTextEdit::MyTextEdit(QWidget *parent): QTextEdit(parent)
{

}

/**
 * ctrl + 滚轮实现字体放大缩小
 * @brief MyTextEdit::wheelEvent
 * @param e
 */
void MyTextEdit::wheelEvent(QWheelEvent *e)
{
    if (ctrlKeyPressed) {
        if (e->angleDelta().y() > 0) {
            zoomIn();
        } else if (e->angleDelta().y() < 0) {
            zoomOut();
        }
        e->accept();
    } else {
        QTextEdit::wheelEvent(e);
    }
}

void MyTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        ctrlKeyPressed = 1;
    }
    QTextEdit::keyPressEvent(e);
}

void MyTextEdit::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        ctrlKeyPressed = 0;
    }
    QTextEdit::keyReleaseEvent(e);
}
