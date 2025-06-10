#ifndef WIDGET_H
#define WIDGET_H

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPort>
#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_btnCloseOrOpenSerial_clicked();

    void on_btnSend_clicked();

    void on_SerialData_readyToRead();

    void on_checkBoxSendIntime_clicked(bool checked);

    void openSerialOptions();

    void closeSerialOptions();

    void on_btnClearRec_clicked();

    void on_btnSaveRec_clicked();

    void on_checkBoxRecTime_clicked(bool checked);

    void on_checkBoxHexDisplay_clicked(bool checked);

    void on_btnHidePanel_clicked(bool checked);

    void on_btnHideHistory_clicked(bool checked);

    void refreshSerialName();

    void on_command_button_clicked();

    void on_checkBox_clicked(bool checked);

    void on_btnReset_clicked();

    void on_btnSave_clicked();

    void on_btnLoad_clicked();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;
    int writeCntTotal;  // 发送字节数
    int readCntTotal;   // 接收字节数
    QString sendBack;
    bool serialOpen;   // 串口打开状态
    QTimer *sendTimer;   // 定时发送定时器
    QTimer *currentTimeTimer;    // 当前时间定时器
    bool showRecTime;   // 是否显示接收时间
    QList<QPushButton*> buttons;    // 多文本按钮
    QList<QLineEdit*> lineEdits;
    QList<QCheckBox*> checkBoxs;

    QString getCurrentTime();   // 获取当前时间


};
#endif // WIDGET_H
