
#ifndef RUUVI_ESP32_MEASUREMENT_H
#define RUUVI_ESP32_MEASUREMENT_H
#include <sys/time.h>

typedef struct measurement_t {
    time_t timestamp;
    char name[25];
    char bda[18];
    int ble_rssi;
    double temperature;
    double humidity;
    unsigned int pressure;
    double acceleration_x;
    double acceleration_y;
    double acceleration_z;
    double battery;
    int txpower;
    unsigned int moves;
    unsigned int sequence;
} measurement_t;

#endif