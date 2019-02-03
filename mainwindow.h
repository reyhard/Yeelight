#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QHostInfo>
#include <QtNetwork>
#include <stdint.h>
#include <bulb_t.h>
#include <vector>
#include <algorithm>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    int sub_string(QByteArray &start_str, QByteArray &end_str, QByteArray &rtn_str);
    int return_value(QList<QString> value, QStringList &rtn_str);
    int update_mode();
    //int slider_brightness(int);

private slots:
    void processPendingDatagrams();
    void on_pushButton_clicked();

    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_check_clicked();
    void on_pushButton_switch_rgb_clicked();
    void on_pushButton_switch_temp_clicked();
    void on_pushButton_switch_hvt_clicked();

    // Brightness slider
    void on_horizontalSlider_valueChanged(int value);
    // HVT slider
    void on_slider_hue_valueChanged(int value);
    void on_slider_saturation_valueChanged(int value);
    // Color temperature slider
    void on_slider_ct_valueChanged(int value);
    // RGB sliders
    void on_slider_rgb_red_valueChanged(int value);
    void on_slider_rgb_green_valueChanged(int value);
    void on_slider_rgb_blue_valueChanged(int value);

    void readyRead();

private:
    Ui::MainWindow *ui;
    QUdpSocket udp_socket;
    uint16_t udp_port;
    QByteArray udp_datagram_recv;

    QTcpSocket tcp_socket;
    QTcpSocket *socket;
    uint16_t tcp_port;

    vector<bulb_t> bulb;
    vector<bulb_t>::iterator ib;

    QHostAddress mcast_addr;
    QString local_ip;
    QString bulb_ip;
    QString bulb_bright;
    QString bulb_id_str;
    QString bulb_temp;
    QString bulb_hue;
    QString bulb_sat;
    QString bulb_mode;
    QString dataString;
};

#endif // MAINWINDOW_H
