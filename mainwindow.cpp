﻿#include "mainwindow.h"
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

    QByteArray stream= tcp_socket.readAll();
    dataString = stream;
    qDebug() << dataString;
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

int MainWindow::return_value(QList<QString> value, QByteArray &rtn_str)
{//
    QByteArray result;
    QString paramsString = "";

    int device_idx = ui->comboBox->currentIndex();
    QString device_idx_str = bulb[device_idx].get_id_str().c_str();

    // Merge all params into single string
    foreach(QString str, value)
    {
        paramsString.append(QString("\"%1\",").arg(str));
    };
    paramsString.chop(1);

    // Compose message to send
    QString parsedString = QString("{\"id\":%1,\"method\":\"get_prop\",\"params\":[%2]}\r\n").arg(device_idx_str,paramsString);
    QByteArray cmd_str = parsedString.toUtf8();

    qDebug()<<"params parsed" <<parsedString;
    // Send message
   // qDebug() << "write size" << tcp_socket.write(IntToArray(cmd_str.size()));
    qDebug() << "retr1" << tcp_socket.write(cmd_str.data());
    // Wait for response
    tcp_socket.waitForReadyRead(1000);
    qDebug()<<"params retrived" <<dataString;
    qDebug()<<"params retrived" <<tcp_socket.readAll();

    // Parse returned message
    QString expectedString = QString("{\"id\":%1, \"result\"}").arg(device_idx_str);

    rtn_str = result;

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
        qDebug() << cmd_str->data();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }
}

void MainWindow::on_pushButton_check_clicked()
{   // check status button
    QByteArray *cmd_str =new QByteArray;
    cmd_str->clear();

    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        QByteArray start_str;
        QByteArray end_str;
        return_value(QList<QString>{"power","ct","hue","bright"},end_str);
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
/* */
    QByteArray *cmd_str =new QByteArray;
    cmd_str->clear();
    cmd_str->append("{\"id\":");

    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        cmd_str->append(bulb[device_idx].get_id_str().c_str());
        qDebug() << "combox index  = " << device_idx;

        cmd_str->append(",\"method\":\"set_bright\",\"params\":[");
		cmd_str->append(QString("%1").arg(pos));
        cmd_str->append(", \"smooth\", 500]}\r\n");
        tcp_socket.write(cmd_str->data());
        qDebug() << cmd_str->data();

        /*QString testString = QString("{\"id\":%1,\"method\":\"get_prop\",\"params\":[\"power\", \"ct\", \"hue\", \"bright\"]}\r\n").arg(bulb[device_idx].get_id_str().c_str());

        qDebug()<<"params retrived" <<testString;
        cmd_str->clear();
        cmd_str->append("{\"id\":");
        cmd_str->append(bulb[device_idx].get_id_str().c_str());
        cmd_str->append(",\"method\":\"get_prop\",\"params\":[\"power\", \"ct\", \"hue\", \"bright\"]}\r\n");
        qDebug() << "retr2" << tcp_socket.write(cmd_str->data());\
        qDebug()<<"params retrived" <<tcp_socket.readLine( 128);*/
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
/* */
    QByteArray *cmd_str =new QByteArray;
    cmd_str->clear();
    cmd_str->append("{\"id\":");

    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        cmd_str->append(bulb[device_idx].get_id_str().c_str());
        qDebug() << "combox index  = " << device_idx;

        cmd_str->append(",\"method\":\"set_hsv\",\"params\":[");
        cmd_str->append(QString("%1").arg(posHue));
        cmd_str->append(",");
        cmd_str->append(QString("%1").arg(posSat));
        cmd_str->append(", \"smooth\", 500]}\r\n");
        //tcp_socket.write(cmd_str->data());
        qDebug() << cmd_str->data();
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
/* */
    QByteArray *cmd_str =new QByteArray;
    cmd_str->clear();
    cmd_str->append("{\"id\":");

    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        cmd_str->append(bulb[device_idx].get_id_str().c_str());
        qDebug() << "combox index  = " << device_idx;

        cmd_str->append(",\"method\":\"set_hsv\",\"params\":[");
        cmd_str->append(QString("%1").arg(posHue));
        cmd_str->append(",");
        cmd_str->append(QString("%1").arg(posSat));
        cmd_str->append(", \"smooth\", 500]}\r\n");
        //tcp_socket.write(cmd_str->data());
        qDebug() << cmd_str->data();
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

    QByteArray *cmd_str =new QByteArray;
    cmd_str->clear();
    cmd_str->append("{\"id\":");

    int device_idx = ui->comboBox->currentIndex();
    if(bulb.size() > 0)
    {
        cmd_str->append(bulb[device_idx].get_id_str().c_str());
        qDebug() << "combox index  = " << device_idx;

        QString testString = QString("{\"id\":%1,\"method\":\"set_ct_abx\",\"params\":[%2, \"smooth\", 500]}\r\n").arg(bulb[device_idx].get_id_str().c_str(),pos);

        qDebug()<<"params retrived" <<testString;

        cmd_str->append(",\"method\":\"set_ct_abx\",\"params\":[");
        cmd_str->append(QString("%1").arg(pos));
        cmd_str->append(", \"smooth\", 500]}\r\n");
        tcp_socket.write(cmd_str->data());
        qDebug() << cmd_str->data();
    }
    else
    {
        qDebug()<<"Bulb is empty";
    }

}