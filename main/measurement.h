
#ifndef RUUVI_ESP32_MEASUREMENT_H
#define RUUVI_ESP32_MEASUREMENT_H
#include <sys/time.h>

typedef struct measurement_t {
    time_t timestamp;
    char bda[18];
    char name[25];
    int ble_rssi;
    float temperature;
    float humidity;
    float pressure;
    float acceleration_x;
    float acceleration_y;
    float acceleration_z;
    float battery;
    int tx_power;
    unsigned int moves;
    unsigned int sequence;
} measurement_t;

#endif