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

    // �����ڱ仯ʱ�����ּ����ӿؼ���֮����
    this->setLayout(ui->verticalLayout);

    // �����ڱ仯ʱ���ײ���̬����
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
 * �Ŵ�����
 * @brief Widget::zoomIn
 */
void Widget::zoomIn() {
    // ���������Ϣ
    QFont font = ui->textEdit->font();
    // ��õ�ǰ����Ĵ�С
    int fontSize = font.pointSize();
    if(fontSize == -1) return;
    // �Ŵ�
    font.setPointSize(fontSize + 1);
    ui->textEdit->setFont(font);
}


/**
 * ��С����
 * @brief Widget::zoomOut
 */
void Widget::zoomOut() {
    // ���������Ϣ
    QFont font = ui->textEdit->font();
    // ��õ�ǰ����Ĵ�С
    int fontSize = font.pointSize();
    if(fontSize == -1) return;
    // ��С
    font.setPointSize(fontSize - 1);
    ui->textEdit->setFont(font);
}


/**
 * �ر�Ӧ�ó���
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
 * ���ļ�
 * @brief Widget::on_btnOpen_clicked
 */
void Widget::on_btnOpen_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    "D:/project/Qt_cpp/code/demo-notebook",
                                                    tr("Text (*.txt)"));
    // �������
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
 * �����ļ�
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

    // ÿ�α���ǰ�ر��ļ����������滻ģʽ��
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
 * �ر��ļ�
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
 * ��ѡ��ѡ��仯ʱ����
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
 * ���λ�÷����仯ʱ����
 * @brief Widget::on_textEdit_cursorPositionChanged
 */
void Widget::on_textEdit_cursorPositionChanged()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    QString blockNum = QString::number(cursor.blockNumber() + 1);
    QString colNum = QString::number(cursor.columnNumber() + 1);
    QString labelMessage = "L:" + blockNum + ",C:" + colNum;
    ui->labelPosition->setText(labelMessage);

    // ���õ�ǰ�кŶ�Ӧ�������и���
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection ext;
    ext.format.setBackground(Qt::yellow);
    ext.format.setProperty(QTextFormat::FullWidthSelection, true);

    // ��ȡ��ǰ�飨�߼��У�
    QTextBlock currentBlock = cursor.block();

    // ����һ��ѡ�����򣬰���������
    QTextCursor highlightCursor(ui->textEdit->document());
    // ���ø�����ʼλ��
    highlightCursor.setPosition(currentBlock.position());

    // �����������λ�ã���������һ���飬�������ĵ�ĩβ����������ڿ��ĩβ
    if (currentBlock.next().isValid()) {
        // �������һ���飺ʹ�ÿ�Ľ���λ��
        highlightCursor.setPosition(currentBlock.position() + currentBlock.length(), QTextCursor::KeepAnchor);
    } else {
        // �����һ���飺ʹ���ĵ����ܳ�����Ϊ����λ��
        highlightCursor.setPosition(ui->textEdit->document()->characterCount() - 1, QTextCursor::KeepAnchor);
    }

    ext.cursor = highlightCursor;
    extraSelections.append(ext);
    ui->textEdit->setExtraSelections(extraSelections);
}
