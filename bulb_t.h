#ifndef BULT_T_H
#define BULT_T_H

#include <iostream>
#include <string>

using namespace std;
class bulb_t
{
    string ip_str;
    static int port;
    string id_str;

    int brightness;
    int temperature;
    int hue;
    int saturation;
public:
    bulb_t(void);
    bulb_t(string ip, string id, int brt = 100, int temp = 1500, int hue_v = 0, int sat = 0, int pt = 55443);
    string get_ip_str();
    string get_id_str();
    int get_port();
    bool operator == (const bulb_t &x) { return (this->ip_str == x.ip_str) && (this->port == x.port) && (this->id_str == x.id_str); }
    void set_brightness(int brn_value);
    int get_brightness();
    int get_temperature();
    int get_hue();
    int get_saturation();
};

#endif // BULT_T_H
