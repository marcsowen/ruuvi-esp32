//
// Created by Marc Sowen on 28.12.24.
//

#ifndef RUUVI_ESP32_HTTP_H
#define RUUVI_ESP32_HTTP_H

#include <esp_http_server.h>
#include "measurement.h"

httpd_handle_t start_webserver(void);
void update_sensor(measurement_t new_measurement);

#endif
