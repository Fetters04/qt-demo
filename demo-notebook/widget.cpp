#include "widget.h"
#include "ui_widget.h"

#include <QFile>
#include <QDebug>
#include <QFileDialog>
#include <QTextBlock>
#include <QMessageBox>
#include <QShortcut>

Widget::Widget(QWidget *parent): QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 当窗口变化时，布局及其子控件随之调整
    this->setLayout(ui->verticalLayout);

    // 当窗口变化时，底部动态调整
    ui->widgetBottom->setLayout(ui->horizontalLayoutBottom);

    QShortcut *shortcutOpen = new QShortcut(QKeySequence(tr("Ctrl+O", "File|Open")), this);
    QShortcut *shortcutSave = new QShortcut(QKeySequence(tr("Ctrl+S", "File|Save")), this);
    QShortcut *shortcutZoomIn = new QShortcut(QKeySequence(tr("Ctrl+Shift+=", "File|Save")), this);
    QShortcut *shortcutZoomOut = new QShortcut(QKeySequence(tr("Ctrl+Shift+-", "File|Save")), this);

    connect(shortcutOpen, &QShortcut::activated, [=](){
        on_btnOpen_clicked();
    });
    connect(shortcutSave, &QShortcut::activated, [=](){
        on_btnSave_clicked();
    });
    connect(shortcutZoomIn, &QShortcut::activated, [=](){
        zoomIn();
    });
    connect(shortcutZoomOut, &QShortcut::activated, [=](){
        zoomOut();
    });
}

Widget::~Widget()
{
    delete ui;
}


/**
 * 放大字体
 * @brief Widget::zoomIn
 */
void Widget::zoomIn() {
    // 获得字体信息
    QFont font = ui->textEdit->font();
    // 获得当前字体的大小
    int fontSize = font.pointSize();
    if(fontSize == -1) return;
    // 放大
    font.setPointSize(fontSize + 1);
    ui->textEdit->setFont(font);
}


/**
 * 缩小字体
 * @brief Widget::zoomOut
 */
void Widget::zoomOut() {
    // 获得字体信息
    QFont font = ui->textEdit->font();
    // 获得当前字体的大小
    int fontSize = font.pointSize();
    if(fontSize == -1) return;
    // 缩小
    font.setPointSize(fontSize - 1);
    ui->textEdit->setFont(font);
}


/**
 * 关闭应用程序
 * @brief Widget::closeEvent
 * @param event
 */
void Widget::closeEvent(QCloseEvent *event)
{
    int ret = QMessageBox::warning(this, tr("My Application"),
                                   tr("close the window\n"
                                      "Do you want to close the window?"),
                                   QMessageBox::Ok | QMessageBox::No);
    switch (ret) {
    case QMessageBox::Ok:
        event->accept();
        break;
    case QMessageBox::No:
        event->ignore();
        break;
    }
}


/**
 * 打开文件
 * @brief Widget::on_btnOpen_clicked
 */
void Widget::on_btnOpen_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "D:/project/Qt_cpp/code/demo-notebook",
                                                    tr("Text (*.txt)"));
    // 清空内容
    ui->textEdit->clear();

    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "file open error";
    }

    this->setWindowTitle(fileName + " NoteBook");

    QTextStream in(&file);
    QString codec = ui->comboBox->currentText();
    in.setCodec(codec.toStdString().c_str());
    while(!in.atEnd()) {
        QString context = in.readLine();
        ui->textEdit->append(context);
    }
}


/**
 * 保存文件
 * @brief Widget::on_btnSave_clicked
 */
void Widget::on_btnSave_clicked()
{
    if(!file.isOpen()) {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                        "D:/project/Qt_cpp/code/demo-notebook/untitled.txt",
                                                        tr("Text (*.txt)"));
        file.setFileName(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "file open error";
        }

        this->setWindowTitle(fileName + " NoteBook");
    }

    // 每次保存前关闭文件并重新以替换模式打开
    if(file.isOpen()) file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "file open error";
    }

    QTextStream out(&file);
    out.setCodec(ui->comboBox->currentText().toStdString().c_str());
    QString context = ui->textEdit->toPlainText();
    out << context;
}


/**
 * 关闭文件
 * @brief Widget::on_btnClose_clicked
 */
void Widget::on_btnClose_clicked()
{
    int ret = QMessageBox::warning(this, tr("My Application"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard
                                   | QMessageBox::Cancel,
                                   QMessageBox::Save);

    switch (ret) {
    case QMessageBox::Save:
        on_btnSave_clicked();
        break;
    case QMessageBox::Discard:
        ui->textEdit->clear();
        if(file.isOpen()) {
            file.close();
            this->setWindowTitle("NoteBook");
        }
        break;
    case QMessageBox::Cancel:
        // Cancel was clicked
        break;
    default:
        // should never be reached
        break;
    }
}


/**
 * 多选框选择变化时触发
 * @brief Widget::on_comboBox_currentIndexChanged
 * @param index
 */
void Widget::on_comboBox_currentIndexChanged(int index)
{
    if(file.isOpen()) {
        ui->textEdit->clear();
        QTextStream in(&file);
        in.setCodec(ui->comboBox->currentText().toStdString().c_str());
        file.seek(0);
        while (!in.atEnd()) {
            QString context = in.readLine();
            ui->textEdit->append(context);
        }
    }
}


/**
 * 光标位置发生变化时触发
 * @brief Widget::on_textEdit_cursorPositionChanged
 */
void Widget::on_textEdit_cursorPositionChanged()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    QString blockNum = QString::number(cursor.blockNumber() + 1);
    QString colNum = QString::number(cursor.columnNumber() + 1);
    QString labelMessage = "L:" + blockNum + ",C:" + colNum;
    ui->labelPosition->setText(labelMessage);

    // 设置当前行号对应的所有行高亮
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection ext;
    ext.format.setBackground(Qt::yellow);
    ext.format.setProperty(QTextFormat::FullWidthSelection, true);

    // 获取当前块（逻辑行）
    QTextBlock currentBlock = cursor.block();

    // 创建一个选择区域，包含整个块
    QTextCursor highlightCursor(ui->textEdit->document());
    // 设置高亮起始位置
    highlightCursor.setPosition(currentBlock.position());

    // 计算高亮结束位置：如果是最后一个块，结束于文档末尾；否则结束于块的末尾
    if (currentBlock.next().isValid()) {
        // 不是最后一个块：使用块的结束位置
        highlightCursor.setPosition(currentBlock.position() + currentBlock.length(), QTextCursor::KeepAnchor);
    } else {
        // 是最后一个块：使用文档的总长度作为结束位置
        highlightCursor.setPosition(ui->textEdit->document()->characterCount() - 1, QTextCursor::KeepAnchor);
    }

    ext.cursor = highlightCursor;
    extraSelections.append(ext);
    ui->textEdit->setExtraSelections(extraSelections);
}
