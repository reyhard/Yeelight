#include "bulb_t.h"

int bulb_t::port = 55443;

bulb_t::bulb_t(void)
{
    ip_str.clear();
    id_str.clear();
    port = 55443;

    brightness = 100;
    temperature = 1000;
    hue = 0;
    saturation = 0;
}

bulb_t::bulb_t(string ip, string id, int brt,int temp,int hue_v, int sat, int pt /*= 55443*/)
{
    ip_str = ip;
    id_str = id;
    port = pt;

    brightness = brt;
    temperature = temp;
    hue = hue;
    saturation = sat;
}

std::string bulb_t::get_ip_str()
{
    return ip_str;
}

std::string bulb_t::get_id_str()
{
    return id_str;
}

int bulb_t::get_port()
{
    return port;
}

void bulb_t::set_brightness(int brn_value)
{
    brightness = brn_value;
}

int bulb_t::get_brightness()
{
    return brightness;
}
int bulb_t::get_temperature()
{
    return temperature;
}

int bulb_t::get_hue()
{
    return hue;
}

int bulb_t::get_saturation()
{
    return saturation;
}

