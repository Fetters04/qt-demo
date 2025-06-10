#include "widget.h"
#include "ui_widget.h"
#include "mycombobox.h"

#include <QSerialPortInfo>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QThread>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 页面缩放自适应
    this->setLayout(ui->gridLayoutGlobal);

    // 初始化
    serialPort = new QSerialPort(this);
    writeCntTotal = 0;
    readCntTotal = 0;
    serialOpen = false;
    sendTimer = new QTimer(this);
    currentTimeTimer = new QTimer(this);
    showRecTime = false;

    // 控件初始化
    ui->btnSend->setDisabled(true);
    ui->checkBoxSendIntime->setDisabled(true);
    ui->lineEditTime->setDisabled(true);
    ui->checkBoxSendNewline->setDisabled(true);
    ui->checkBoxHexSend->setDisabled(true);

    // 获取当前时间并显示，启动计时器开启动态刷新
    ui->labelCurrentTime->setText(getCurrentTime());
    currentTimeTimer->start(1000);

    connect(serialPort, &QSerialPort::readyRead, this, &Widget::on_SerialData_readyToRead);
    // 定时发送
    connect(sendTimer, &QTimer::timeout, [=]() {
        on_btnSend_clicked();
    });
    // 动态刷新当前时间
    connect(currentTimeTimer, &QTimer::timeout, [=]() {
        // 获取当前时间并显示
        ui->labelCurrentTime->setText(getCurrentTime());
    });
    // 串口号刷新
    connect(ui->comboBoxSerial, &MyComboBox::refresh, this, &Widget::refreshSerialName);

    ui->comboBoxBaudRate->setCurrentIndex(6);
    ui->comboBoxDatabit->setCurrentIndex(3);

    refreshSerialName();

    ui->labelSendState->setText(ui->comboBoxSerial->currentText() + " NotOpen!");

    // 多文本按钮处理
    for (int i = 1; i <= 9; i++) {
        QString btnName = QString("pushButton_%1").arg(i);
        QPushButton* btn = findChild<QPushButton*>(btnName);
        if (btn) {
            btn->setProperty("buttonId", i);
            buttons.append(btn);
            connect(btn, SIGNAL(clicked()), this, SLOT(on_command_button_clicked()));
        }

        QString lineEditName = QString("lineEdit_%1").arg(i);
        QLineEdit* lineEdit = findChild<QLineEdit*>(lineEditName);
        lineEdits.append(lineEdit);

        QString checkBoxName = QString("checkBox_%1").arg(i);
        QCheckBox* checkBox = findChild<QCheckBox*>(checkBoxName);
        checkBoxs.append(checkBox);
    }
}

Widget::~Widget()
{
    delete ui;
}

/**
 * 打开或关闭串口
 * @brief Widget::on_btnCloseOrOpenSerial_clicked
 */
void Widget::on_btnCloseOrOpenSerial_clicked()
{
    if (!serialOpen) {
        // 1.选择端口
        serialPort->setPortName(ui->comboBoxSerial->currentText());

        // 2.配置波特率
        serialPort->setBaudRate(ui->comboBoxBaudRate->currentText().toInt());

        // 3.配置数据位
        serialPort->setDataBits(QSerialPort::DataBits(ui->comboBoxDatabit->currentText().toInt()));

        // 4.配置校验位
        switch (ui->comboBoxCheckbit->currentIndex()) {
        case 0:
            serialPort->setParity(QSerialPort::NoParity);
            break;
        case 1:
            serialPort->setParity(QSerialPort::EvenParity);
            break;
        case 2:
            serialPort->setParity(QSerialPort::MarkParity);
            break;
        case 3:
            serialPort->setParity(QSerialPort::OddParity);
            break;
        case 4:
            serialPort->setParity(QSerialPort::SpaceParity);
            break;
        default:
            serialPort->setParity(QSerialPort::UnknownParity);
            break;
        }

        // 5.配置停止位
        serialPort->setStopBits(QSerialPort::StopBits(ui->comboBoxStopbit->currentText().toInt()));

        // 6.流控
        if (ui->comboBoxFlowCon->currentText() == "None")
            serialPort->setFlowControl(QSerialPort::NoFlowControl);

        // 7.打开串口
        if (serialPort->open(QIODevice::ReadWrite)) {
            openSerialOptions();
            serialOpen = true;
            ui->labelSendState->setText(ui->comboBoxSerial->currentText() + " isOpen!");
        } else {
            QMessageBox msgBox;
            msgBox.setWindowTitle("打开串口错误");
            msgBox.setText("打开失败，串口可能被占用或者已拔出");
            msgBox.exec();
        }
    } else {
        // 关闭串口
        serialPort->close();
        sendTimer->stop();
        closeSerialOptions();
        serialOpen = false;
        ui->labelSendState->setText(ui->comboBoxSerial->currentText() + " isClose!");
    }
}

/**
 * 发送消息
 * @brief Widget::on_btnSend_clicked
 */
void Widget::on_btnSend_clicked()
{
    int writeCnt = 0;
    // 读取内容
    const char* sendData = ui->lineEditSendContext->text().toLocal8Bit().constData();

    // 使用HEX发送
    if (ui->checkBoxHexSend->isChecked()) {
        QString tmp = ui->lineEditSendContext->text();
        // 判断是否是偶数位
        QByteArray tmpQBA = tmp.toLocal8Bit();
        if (tmpQBA.size() % 2 != 0) {
            ui->labelSendState->setText("Error Input!");
            return;
        }
        // 判断是否符合16进制的表达
        for (char c: tmpQBA) {
            if (!std::isxdigit(c)) {
                ui->labelSendState->setText("Error Input!");
                return;
            }
        }
        // 转换为16进制发送，拒接变为字符通过ASCII发送
        QByteArray arraySend = QByteArray::fromHex(tmpQBA);

        // 勾选"发送新行"
        if (ui->checkBoxSendNewline->isChecked()) {
            arraySend.append("\r\n");
        }
        writeCnt = serialPort->write(arraySend);

    } else {
        // 勾选"发送新行"
        if (ui->checkBoxSendNewline->isChecked()) {
            QByteArray arraySendData(sendData, strlen(sendData));
            arraySendData.append("\r\n");
            writeCnt = serialPort->write(arraySendData);
        } else {
            writeCnt = serialPort->write(sendData);
        }
    }

    // 发送失败
    if (writeCnt == -1) {
        ui->labelSendState->setText("Send Error!");
    } else {
        // 累计发送数据量
        writeCntTotal += writeCnt;

        // 更新发送状态，更新发送总数据量
        ui->labelSendState->setText("Send OK!");
        ui->labelSentNum->setText("Sent:" + QString::number(writeCntTotal));

        // 发送记录与备份不等才追加到历史记录
        if (strcmp(sendData, sendBack.toStdString().c_str())) {
            ui->textEditHistory->append(sendData);
            sendBack = QString::fromUtf8(sendData);
        }
    }
}

/**
 * 接收消息
 * @brief Widget::on_SerialData_readyToRead
 */
void Widget::on_SerialData_readyToRead()
{
    QString recMessage = serialPort->readAll();
    if (recMessage != NULL) {
        // 勾选"自动换行"
        if (ui->checkBoxAutoLine->isChecked()) recMessage.append("\r\n");

        // 如果勾选了"HEX显示"
        if (ui->checkBoxHexDisplay->isChecked()) {
            // 将新读入的hex消息追加
            QByteArray hexQBA = recMessage.toUtf8().toHex().toUpper();
            ui->textEditRec->setText(QString::fromUtf8(hexQBA));
        } else {
            // 如果勾选了"接收时间"
            if (showRecTime) {
                recMessage = "【" + getCurrentTime() + "】" + recMessage;
            }
            ui->textEditRec->insertPlainText(recMessage);
        }

        // 统计接收字节数
        readCntTotal += recMessage.size();
        ui->labelReceivedNum->setText("Received:" + QString::number(readCntTotal));
    }
}

/**
 * 定时发送功能
 * @brief Widget::on_checkBoxSendIntime_clicked
 * @param checked 是否勾选"定时发送"
 */
void Widget::on_checkBoxSendIntime_clicked(bool checked)
{
    if (checked) {
        // 如果串口未打开，不执行
        if (!serialOpen) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("串口未打开");
            msgBox.setText("发送前请先打开串口");
            msgBox.exec();
            return;
        }

        // 禁用组件
        ui->lineEditTime->setDisabled(true);
        ui->btnSend->setDisabled(true);

        // 开启定时器
        sendTimer->start(ui->lineEditTime->text().toInt());
    } else {
        ui->lineEditTime->setDisabled(false);
        ui->btnSend->setDisabled(false);
        // 关闭定时器
        sendTimer->stop();
    }
}

/**
 * 打开串口对组件的一系列操作
 * @brief openSerialOptions
 */
void Widget::openSerialOptions() {
    // 参数配置禁用
    ui->btnCloseOrOpenSerial->setText("关闭串口");
    ui->comboBoxSerial->setDisabled(true);
    ui->comboBoxBaudRate->setDisabled(true);
    ui->comboBoxDatabit->setDisabled(true);
    ui->comboBoxCheckbit->setDisabled(true);
    ui->comboBoxStopbit->setDisabled(true);
    ui->comboBoxFlowCon->setDisabled(true);

    // 可用发送功能
    ui->btnSend->setEnabled(true);
    ui->checkBoxSendIntime->setEnabled(true);

    ui->checkBoxSendNewline->setDisabled(false);
    ui->checkBoxHexSend->setDisabled(false);
}

/**
 * 关闭串口对组件的一系列操作
 * @brief closeSerialOptions
 */
void Widget::closeSerialOptions() {
    // 参数配置可用
    ui->btnCloseOrOpenSerial->setText("打开串口");
    ui->comboBoxSerial->setDisabled(false);
    ui->comboBoxBaudRate->setDisabled(false);
    ui->comboBoxDatabit->setDisabled(false);
    ui->comboBoxCheckbit->setDisabled(false);
    ui->comboBoxStopbit->setDisabled(false);
    ui->comboBoxFlowCon->setDisabled(false);

    // 禁用发送功能，解除勾选状态
    ui->btnSend->setDisabled(true);
    ui->checkBoxSendIntime->setDisabled(true);
    ui->checkBoxSendIntime->setCheckState(Qt::Unchecked);
    ui->lineEditTime->setDisabled(true);
    ui->lineEditSendContext->setDisabled(false);

    ui->checkBoxSendNewline->setDisabled(true);
    ui->checkBoxHexSend->setDisabled(true);
}

/**
 * 清空接收
 * @brief Widget::on_btnClearRec_clicked
 */
void Widget::on_btnClearRec_clicked()
{
    ui->textEditRec->setText("");
}

/**
 * 保存接收
 * @brief Widget::on_btnSaveRec_clicked
 */
void Widget::on_btnSaveRec_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                    "D:/project/Qt_cpp/code/demo-serial/serialReceivedData.txt",
                                                    tr("Text (*.txt)"));
    if (fileName != NULL) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
        QTextStream out(&file);
        out << ui->textEditRec->toPlainText();
        file.close();
    }
}

/**
 * 获取当前时间
 * @brief Widget::getCurrentTime
 * @return 格式化处理的当前时间
 */
QString Widget::getCurrentTime()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd HH:mm:ss");
    return formattedDateTime;
}

/**
 * 在接收消息中显示时间
 * @brief Widget::on_checkBoxRecTime_clicked
 * @param checked 是否勾选"接收时间"
 */
void Widget::on_checkBoxRecTime_clicked(bool checked)
{
    if (checked) {
        showRecTime = true;
    } else {
        showRecTime = false;
    }
}

/**
 * HEX显示
 * @brief Widget::on_checkBoxHexDisplay_clicked
 * @param checked 是否勾选"HEX显示"
 */
void Widget::on_checkBoxHexDisplay_clicked(bool checked)
{
    if (checked) {
        // 读取textEditRec内容
        QString tmpString = ui->textEditRec->toPlainText();

        // 转换为hex
        QByteArray tmpQBA = tmpString.toUtf8();
        QByteArray hexQBA = tmpQBA.toHex();

        // 显示
        QString lastShow;
        tmpString = QString::fromUtf8(hexQBA);
        for (int i = 0; i < tmpString.size(); i+=2) {
            lastShow += tmpString.mid(i, 2) + " ";
        }
        ui->textEditRec->setText(lastShow.toUpper());

        // 禁用"接收时间"
        ui->checkBoxRecTime->setCheckState(Qt::Unchecked);
        ui->checkBoxRecTime->setDisabled(true);
    } else {
        // 读取textEditRec内容（hex显示）
        QString hexString = ui->textEditRec->toPlainText();
        QByteArray hexQBA = hexString.toUtf8();
        QByteArray tmpQBA = QByteArray::fromHex(hexQBA);

        // 显示
        ui->textEditRec->setText(QString::fromUtf8(tmpQBA));
        // 解禁"接收时间"
        ui->checkBoxRecTime->setDisabled(false);
    }
}

/**
 * 隐藏面板
 * @brief Widget::on_btnHidePanel_clicked
 * @param checked
 */
void Widget::on_btnHidePanel_clicked(bool checked)
{
    if (checked) {
        ui->btnHidePanel->setText("拓展面板");
        ui->groupBoxTexts->hide();
    } else {
        ui->btnHidePanel->setText("隐藏面板");
        ui->groupBoxTexts->show();
    }
}

/**
 * 隐藏历史
 * @brief Widget::on_btnHideHistory_clicked
 * @param checked
 */
void Widget::on_btnHideHistory_clicked(bool checked)
{
    if (checked) {
        ui->btnHideHistory->setText("显示历史");
        ui->groupBoxHistory->hide();
    } else {
        ui->btnHideHistory->setText("隐藏历史");
        ui->groupBoxHistory->show();
    }
}

/**
 * 刷新串口
 * @brief Widget::refreshSerialName
 */
void Widget::refreshSerialName() {
    ui->comboBoxSerial->clear();
    // 添加串口项
    QList<QSerialPortInfo> serialList = QSerialPortInfo::availablePorts();
    for(QSerialPortInfo serialInfo: serialList) {
        ui->comboBoxSerial->addItem(serialInfo.portName());
    }
    ui->labelSendState->setText("Com Refreshed!");
}

/**
 * 多文本发送功能实现
 * @brief Widget::on_command_button_clicked
 */
void Widget::on_command_button_clicked()
{
    QPushButton *btn = qobject_cast<QPushButton *>(sender());
    if (btn) {
        // 子控件id
        int id = btn->property("buttonId").toInt();

        QString lineEditName = QString("lineEdit_%1").arg(id);
        QLineEdit* lineEdit = findChild<QLineEdit*>(lineEditName);
        if (!lineEdit || lineEdit->text().size() <= 0) return;
        ui->lineEditSendContext->setText(lineEdit->text());

        QString checkBoxName = QString("checkBox%1").arg(id);
        QCheckBox* checkBox = findChild<QCheckBox*>(checkBoxName);
        if (checkBox) ui->checkBoxHexSend->setChecked(checkBox->isChecked());

        // 发送
        on_btnSend_clicked();
    }
}

/**
 * 循环发送
 * @brief Widget::on_checkBox_clicked
 * @param checked
 */
void Widget::on_checkBox_clicked(bool checked)
{
    if (checked) {
        for (int i = 0; i < 9; i++) {
            QPushButton* btnTmp = buttons[i];
            emit btnTmp->clicked();
            QThread::msleep(ui->spinBox->text().toInt());
        }
    }
}

/**
 * 重置多文本内容
 * @brief Widget::on_btnReset_clicked
 */
void Widget::on_btnReset_clicked()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("提示");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText("重置列表不可逆，确认是否重置？");
    QPushButton *yesButton = msgBox.addButton("是", QMessageBox::YesRole);
    msgBox.exec();
    if (msgBox.clickedButton() == yesButton) {
        for (int i = 0; i < lineEdits.size(); i++) {
            // 清空lineEdit
            lineEdits[i]->clear();
            // 取消勾选checkBox
            checkBoxs[i]->setChecked(false);
        }
    }
}

/**
 * 保存指令集
 * @brief Widget::on_btnSave_clicked
 */
void Widget::on_btnSave_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("保存文件"),
                                                    "D:/project/Qt_cpp/code/demo-serial",
                                                    tr("文本类型 (*.txt)"));
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    for (int i = 0; i < lineEdits.size(); i++) {
        out << checkBoxs[i]->isChecked() << "," << lineEdits[i]->text() << "\n";
    }
    file.close();
}

/**
 * 载入指令集
 * @brief Widget::on_btnLoad_clicked
 */
void Widget::on_btnLoad_clicked()
{
    int i = 0;
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件"),
                                                    "D:/project/Qt_cpp/code/demo-serial",
                                                    tr("文本类型 (*.txt)"));
    if (fileName != NULL) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QTextStream in(&file);
        while (!in.atEnd() && i < 9) {
            QString line = in.readLine();
            QStringList parts = line.split(",");
            if (parts.count() == 2) {
                checkBoxs[i]->setChecked(parts[0].toInt());
                lineEdits[i]->setText(parts[1]);
            }
            i++;
        }
        file.close();
    }
}
