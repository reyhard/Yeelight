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
    string bulb_name;

    int brightness;
    int temperature;
    int hue;
    int saturation;
    int color_mode;
public:
    bulb_t(void);
    bulb_t(string ip, string id, int brt = 100, int temp = 1500, int hue_v = 0, int sat = 0, int mode = 0, string bName = "", int pt = 55443);
    string get_ip_str();
    string get_id_str();
    int get_port();
    bool operator == (const bulb_t &x) { return (this->ip_str == x.ip_str) && (this->port == x.port) && (this->id_str == x.id_str); }
    void set_brightness(int brn_value);
    int get_brightness();
    int get_temperature();
    int get_hue();
    int get_saturation();
    int get_mode();
    void set_mode(int mode_value);
    string get_name();
    void set_name(string bulb_string);
};

#endif // BULT_T_H
