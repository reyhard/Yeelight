#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork/QHostInfo>
#include <QtNetwork>
#include <qbytearray.h>
#include <stdint.h>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //connect( &tcp_socket,SIGNAL(bytesWritten(qint64)),this,SLOT(bytesWritten(qint64)));
    connect( &tcp_socket,SIGNAL(readyRead()),this,SLOT(readyRead()));

    this->setFixedSize(this->width(), this->height());
    MainWindow::setWindowTitle("Yeelight Bulb Toggle");
    //ui->horizontalSlider tracking = false

    mcast_addr = "239.255.255.250";//UDP组播IP
    udp_port = 1982;//UDP port

    {//IP
        QString localHostName = QHostInfo::localHostName();
        QHostInfo info = QHostInfo::fromName(localHostName);
        foreach (QHostAddress address, info.addresses())
        {
            if(address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                ui->label->setText(address.toString());
                local_ip = address.toString();//存储本机IP
                qDebug()<<"Local PC IP:"<<address.toString();
            }
        }
    }

    {
        udp_socket.close();
        if (false == udp_socket.bind(QHostAddress(local_ip), 0, QUdpSocket::ShareAddress))
        {
            qDebug() << "udp bind failed\n";
            return;
        }
        else
        {
            qDebug() << "udp bind success\n";
        }

        udp_socket.joinMulticastGroup(mcast_addr);
        connect(&udp_socket, SIGNAL(readyRead()), // 数据流过来触发readyRead()信号
            this, SLOT(processPendingDatagrams()));
    }
    on_pushButton_3_clicked();
}

MainWindow::~MainWindow()
{
    tcp_socket.close();
    udp_socket.close();
    delete ui;
}

void MainWindow::readyRead()
{
    // Read stream
    QByteArray stream = tcp_socket.readAll();
    dataString = stream;
    qDebug().noquote()  << "TCP LOG:" << dataString;
};

void MainWindow::processPendingDatagrams()
{
    while (udp_socket.hasPendingDatagrams()) 
    { //
        qDebug()<<"udp receive data";
        udp_datagram_recv.resize(udp_socket.pendingDatagramSize());
        udp_socket.readDatagram(udp_datagram_recv.data(), udp_datagram_recv.size()); //读取数据
        qDebug()<<udp_datagram_recv.data();

        QByteArray start_str;
        QByteArray end_str;
        QByteArray rtn_str;

        //bulb_ip
        start_str.clear(); end_str.clear(); rtn_str.clear();
        start_str.append("Location: yeelight://");
        end_str.append(":");
        sub_string(start_str, end_str, rtn_str);
        if(rtn_str.isEmpty() == false)
        {
            bulb_ip = rtn_str;
        }

        //bulb_id
        start_str.clear(); end_str.clear(); rtn_str.clear();
        start_str.append("id: ");
        end_str.append("\r\n");
        sub_string(start_str, end_str, rtn_str);
        if(rtn_str.isEmpty() == false)
        {
            bulb_id_str = rtn_str;
        }

        //bulb_brightness
        start_str.clear(); end_str.clear(); rtn_str.clear();
        start_str.append("bright: ");
        end_str.append("\r\n");
        sub_string(start_str, end_str, rtn_str);
        if(rtn_str.isEmpty() == false)
        {
            bulb_bright = rtn_str;
        }
        qDebug()<< "bulb brightness: "<< bulb_bright;

        // temperature
        start_str.clear(); end_str.clear(); rtn_str.clear();
        start_str.append("ct: ");
        end_str.append("\r\n");
        sub_string(start_str, end_str, rtn_str);
        if(rtn_str.isEmpty() == false)
        {
            bulb_temp = rtn_str;
        }

        // hue
        start_str.clear(); end_str.clear(); rtn_str.clear();
        start_str.append("hue: ");
        end_str.append("\r\n");
        sub_string(start_str, end_str, rtn_str);
        if(rtn_str.isEmpty() == false)
        {
            bulb_hue = rtn_str;
        }

        // saturation
        start_str.clear(); end_str.clear(); rtn_str.clear();
        start_str.append("sat: ");
        end_str.append("\r\n");
        sub_string(start_str, end_str, rtn_str);
        if(rtn_str.isEmpty() == false)
        {
            bulb_sat = rtn_str;
        }

        // get bulb name
        //
        QString bulbName = bulb_id_str;
        //Filling combo box with bulbs
        bulb_t bulb_tmp(bulb_ip.toStdString(), bulb_id_str.toStdString(), bulb_bright.toInt(), bulb_temp.toInt(), bulb_hue.toInt(), bulb_sat.toInt());
        ib = std::find(bulb.begin(), bulb.end(), bulb_tmp);
        if (ib == bulb.end())
        {
            bulb.push_back(bulb_tmp);

            QStringList items;
            QString tmp;
            tmp = bulb_ip.append(" name:");
            tmp.append("biurko");
            items << tmp;
            ui->comboBox->addItems(items);
        }
    }
    on_pushButton_clicked();
}

int MainWindow::sub_string(QByteArray &start_str, QByteArray &end_str, QByteArray &rtn_str)
{//
    QByteArray result;
    int pos1 = -1;
    int pos2 = -1;

    result.clear();
    pos1 = udp_datagram_recv.indexOf(start_str, 0);
    if (pos1 != -1)
    {
        result = udp_datagram_recv.mid(pos1);
        pos1 = start_str.length();
        result = result.mid(pos1);
        pos2 = result.indexOf(end_str);
        result = result.mid(0, pos2);
    }
    rtn_str = result;

    return 0;
}

int MainWindow::return_value(QList<QString> value, QStringList &rtn_str)
{// function to retrive various properties from bulb using get_prop command.

    int device_idx = ui->comboBox->currentIndex();
    QString device_idx_str = bulb[device_idx].get_id_str().c_str();

    // Merge all params into single string
    QString paramsString = "";
    foreach(QString str, value)
    {
        paramsString.append(QString("\"%1\",").arg(str));
    };
    paramsString.chop(1);

    // Compose message to send
    QString parsedString = QString("{\"id\":%1,\"method\":\"get_prop\",\"params\":[%2]}\r\n").arg(device_idx_str,paramsString);
    QByteArray cmd_str = parsedString.toUtf8();

    // Send message
    qDebug().noquote() << "params parsed" << parsedString << " bytes wrote" << tcp_socket.write(cmd_str.data());
    // Wait for response
    tcp_socket.waitForReadyRead(1000);
    qDebug().noquote() << "params retrived" << dataString;

    // Parse returned message
    QString expectedString = QString("{\"id\":%1, \"result\"}").arg(device_idx_str);
    QString resultStr = dataString.mid(19);
    resultStr.chop(4);
    QStringList returnList = resultStr.split(',');
    qDebug().noquote() << "result" << returnList;

    rtn_str = returnList;

    return 0;
}

int MainWindow::send_command(QString command, QString params)
{// function to retrive various properties from bulb using get_prop command.

    int device_idx = ui->comboBox->currentIndex();
    QString device_idx_str = bulb[device_idx].get_id_str().c_str();

    // Compose message to send
    QString parsedString = QString("{\"id\":%1,\"method\":\"%2\",\"params\":[%3]}\r\n").arg(device_idx_str,command,params);
    QByteArray cmd_str = parsedString.toUtf8();

    // Send message
    qDebug().noquote() << "params parsed" << parsedString << " bytes wrote" << tcp_socket.write(cmd_str.data());
    // Wait for response
    tcp_socket.waitForReadyRead(1000);
    qDebug().noquote() << "params retrived" << dataString;

    return 0;
}
int MainWindow::update_mode()
{// function to retrive various properties from bulb using get_prop command.

    QStringList ParamsRestored;
    return_value(QList<QString>{"power","color_mode"},ParamsRestored);
    qDebug().noquote() << "power status" << ParamsRestored.at(0);
    QString colorMode_str;
    int ModeNumber = ParamsRestored.at(1).mid(1,1).toInt();
    int device_idx = ui->comboBox->currentIndex();
    switch (ModeNumber)
    {
        case 1:
            colorMode_str = "RGB";ui->stackedWidget->setCurrentIndex(2);break;
        case 2:
            colorMode_str = "Color Temperature";ui->stackedWidget->setCurrentIndex(0);break;
        case 3:
            colorMode_str = "HSV";ui->stackedWidget->setCurrentIndex(1);break;
    };
    bulb[device_idx].set_mode(ModeNumber);
    QString bulb_status_str = ParamsRestored.at(0);
    bulb_status_str.remove("\"");
    ui->label_bulb_status->setText(QString("Status: %1 (%2)").arg(bulb_status_str,colorMode_str));
    return 0;
}

void MainWindow::on_pushButton_3_clicked()
{//button
    QByteArray datagram = "M-SEARCH * HTTP/1.1\r\n\
HOST: 239.255.255.250:1982\r\n\
MAN: \"ssdp:discover\"\r\n\
ST: wifi_bulb";

    int ret = udp_socket.writeDatagram(datagram.data(), datagram.size(), mcast_addr, udp_port);
    qDebug()<<"udp write"<<ret<<" bytes";
}

void MainWindow::on_pushButton_clicked()
{// connect button
    tcp_socket.close();//关闭上次的连接
    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
       // socket->connectToHost(QHostAddress(bulb[device_idx].get_ip_str().c_str()), bulb[device_idx].get_port());

        tcp_socket.connectToHost(QHostAddress(bulb[device_idx].get_ip_str().c_str()), bulb[device_idx].get_port());
        QString ip_string = QString("currently connected: %1").arg( bulb[device_idx].get_ip_str().c_str());
        ui->label_ip->setText(ip_string);

        update_mode();

        ui->horizontalSlider->setValue(bulb[device_idx].get_brightness());
        ui->slider_ct->setValue(bulb[device_idx].get_temperature());
        ui->slider_hue->setValue(bulb[device_idx].get_hue());
        ui->slider_saturation->setValue(bulb[device_idx].get_saturation());

    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_pushButton_4_clicked()
{//""button,
    QByteArray *cmd_str =new QByteArray;
    cmd_str->clear();
    cmd_str->append("{\"id\":");

    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        cmd_str->append(bulb[device_idx].get_id_str().c_str());
        qDebug() << "combox index  = " << device_idx;


        cmd_str->append(",\"method\":\"toggle\",\"params\":[]}\r\n");
        tcp_socket.write(cmd_str->data());
        //QThread::msleep(2000);
        tcp_socket.waitForReadyRead(1000);
        update_mode();
        qDebug() << cmd_str->data();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_pushButton_check_clicked()
{   // check status button

    //int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        update_mode();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_pushButton_switch_rgb_clicked()
{   // check status button
    int device_idx = ui->comboBox->currentIndex();
    QString device_idx_str = bulb[device_idx].get_id_str().c_str();
    if(bulb.size() > 0)
    {
        ui->stackedWidget->setCurrentIndex(2);
        bulb[device_idx].set_mode(2);

        // Get RGB parameters
        QStringList ParamsRestored;
        return_value(QList<QString>{"rgb"},ParamsRestored);
        QString RGBValue = ParamsRestored.at(0);
        RGBValue.remove("\"");

        // Compose message to send
        QString parsedString = QString("{\"id\":%1,\"method\":\"set_rgb\",\"params\":[%2,\"smooth\",500]}\r\n").arg(device_idx_str,RGBValue);
        qDebug().noquote() << "parsed string to send" << parsedString;
        QByteArray cmd_str = parsedString.toUtf8();
        qDebug().noquote() << tcp_socket.write(cmd_str.data());
        tcp_socket.waitForReadyRead(1000);

        update_mode();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_pushButton_switch_temp_clicked()
{   // check status button
    int device_idx = ui->comboBox->currentIndex();
    QString device_idx_str = bulb[device_idx].get_id_str().c_str();
    if(bulb.size() > 0)
    {
        ui->stackedWidget->setCurrentIndex(0);
        bulb[device_idx].set_mode(0);

        // Get Temp parameters
        QStringList ParamsRestored;
        return_value(QList<QString>{"ct"},ParamsRestored);
        QString CTValue = ParamsRestored.at(0);
        CTValue.remove("\"");

        // Compose message to send
        QString parsedString = QString("{\"id\":%1,\"method\":\"set_ct_abx\",\"params\":[%2,\"smooth\",500]}\r\n").arg(device_idx_str,CTValue);
        qDebug().noquote() << "parsed string to send" << parsedString;
        QByteArray cmd_str = parsedString.toUtf8();
        qDebug().noquote() << tcp_socket.write(cmd_str.data());
        tcp_socket.waitForReadyRead(1000);

        update_mode();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_pushButton_switch_hvt_clicked()
{   // check status button
    int device_idx = ui->comboBox->currentIndex();
    QString device_idx_str = bulb[device_idx].get_id_str().c_str();
    if(bulb.size() > 0)
    {
        ui->stackedWidget->setCurrentIndex(1);
        bulb[device_idx].set_mode(1);

        // Get HSV parameters
        QStringList ParamsRestored;
        return_value(QList<QString>{"hue","sat"},ParamsRestored);
        QString HueValue = ParamsRestored.at(0);
        QString SatValue = ParamsRestored.at(1);
        HueValue.remove("\"");
        SatValue.remove("\"");

        // Compose message to send
        QString parsedString = QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"smooth\",500]}\r\n").arg(device_idx_str,HueValue,SatValue);
        qDebug().noquote() << "parsed string to send" << parsedString;
        QByteArray cmd_str = parsedString.toUtf8();
        qDebug().noquote() << tcp_socket.write(cmd_str.data());
        tcp_socket.waitForReadyRead(1000);

        update_mode();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    int pos = ui->horizontalSlider->value();
    QString slider_value = QString("%1").arg(pos) + "%";
    ui->label_4->setText(slider_value);

    if(bulb.size() > 0)
    {
        send_command("set_bright",QString("%1, \"smooth\", 500").arg(pos));
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }

}

void MainWindow::on_slider_hue_valueChanged(int value)
{
    int posHue = ui->slider_hue->value();
    int posSat = ui->slider_saturation->value();
    QString slider_value = QString("%1").arg(posHue) + "%";
    ui->label_hue->setText(slider_value);

    if(bulb.size() > 0)
    {
        int device_idx = ui->comboBox->currentIndex();
        qDebug()<<"Mode" << bulb[device_idx].get_mode();
        if(bulb[device_idx].get_mode() == 3)
        {
            send_command("set_hsv",QString("%1, %2, \"smooth\", 500").arg(posHue).arg(posSat));
        }
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }

}

void MainWindow::on_slider_saturation_valueChanged(int value)
{
    int posHue = ui->slider_hue->value();
    int posSat = ui->slider_saturation->value();
    QString slider_value = QString("%1").arg(posSat) + "%";
    ui->label_saturation->setText(slider_value);

    if(bulb.size() > 0)
    {
        int device_idx = ui->comboBox->currentIndex();
        qDebug()<<"Mode" << bulb[device_idx].get_mode();
        if(bulb[device_idx].get_mode() == 3)
        {
            send_command("set_hsv",QString("%1, %2, \"smooth\", 500").arg(posHue).arg(posSat));
        }
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }

}

void MainWindow::on_slider_ct_valueChanged(int value)
{
    int pos = ui->slider_ct->value();
    QString slider_value = QString("%1").arg(pos) + "K";
    ui->label_ct->setText(slider_value);

    if(bulb.size() > 0)
    {
        int device_idx = ui->comboBox->currentIndex();
        if(bulb[device_idx].get_mode() == 2)
        {
            send_command("set_ct_abx",QString("%1, \"smooth\", 500").arg(pos));
        }
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }

}

void MainWindow::on_slider_rgb_red_valueChanged(int value)
{
    int pos_red = ui->slider_rgb_red->value();
    int pos_green = ui->slider_rgb_green->value();
    int pos_blue = ui->slider_rgb_blue->value();

    QString slider_value = QString("%1").arg(pos_red) + "K";
    ui->label_rgb_red->setText(slider_value);

    if(bulb.size() > 0)
    {
        int device_idx = ui->comboBox->currentIndex();
        if(bulb[device_idx].get_mode() == 1)
        {
            send_command("set_rgb",QString("%1, \"smooth\", 500").arg(pos_red*65536+pos_green*256+pos_blue));
        }
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_slider_rgb_green_valueChanged(int value)
{

    int pos_red = ui->slider_rgb_red->value();
    int pos_green = ui->slider_rgb_green->value();
    int pos_blue = ui->slider_rgb_blue->value();

    QString slider_value = QString("%1").arg(pos_red) + "K";
    ui->label_rgb_green->setText(slider_value);

    if(bulb.size() > 0)
    {
        int device_idx = ui->comboBox->currentIndex();
        qDebug()<<"Mode" << bulb[device_idx].get_mode();
        if(bulb[device_idx].get_mode() == 1)
        {
            send_command("set_rgb",QString("%1, \"smooth\", 500").arg(pos_red*65536+pos_green*256+pos_blue));
        }
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}
void MainWindow::on_slider_rgb_blue_valueChanged(int value)
{

    int pos_red = ui->slider_rgb_red->value();
    int pos_green = ui->slider_rgb_green->value();
    int pos_blue = ui->slider_rgb_blue->value();

    QString slider_value = QString("%1").arg(pos_red) + "K";
    ui->label_rgb_blue->setText(slider_value);

    if(bulb.size() > 0)
    {
        int device_idx = ui->comboBox->currentIndex();
        if(bulb[device_idx].get_mode() == 1)
        {
            send_command("set_rgb",QString("%1, \"smooth\", 500").arg(pos_red*65536+pos_green*256+pos_blue));
        }
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}
