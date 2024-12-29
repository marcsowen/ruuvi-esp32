
#include <esp_http_server.h>
#include <time.h>
#include <esp_log.h>

#include "measurement.h"

#define MAX_SENSORS 10
#define JSON_BUFFER_SIZE 1024

static const char *TAG = "ruuvi-esp32-http";

measurement_t measurements[MAX_SENSORS];
int sensor_count = 0;

void update_sensor(measurement_t new_measurement) {
    for (int i = 0; i < sensor_count; i++) {
        if (strcmp(measurements[i].bda, new_measurement.bda) == 0) {
            if (measurements[i].sequence == new_measurement.sequence) {
                return; // We already know this sequence number. Skipping.
            }

            measurements[i] = new_measurement;
            ESP_LOGI(TAG, "Updated measurement for sensor %s - #%d", new_measurement.bda, new_measurement.sequence);
            return;
        }
    }

    if (sensor_count < MAX_SENSORS) {
        measurements[sensor_count] = new_measurement;
        sensor_count++;
        ESP_LOGI(TAG, "Added new sensor %s", new_measurement.bda);
    } else {
        ESP_LOGE(TAG, "No more sensor slots available");
    }
}

static esp_err_t request_handler(httpd_req_t *req) {
    char response[JSON_BUFFER_SIZE];
    int offset = 0;
    offset += snprintf(response + offset, JSON_BUFFER_SIZE - offset, "{\"timestamp\": \"%lld\", \"sensors\": [", time(NULL));

    for (int i = 0; i < sensor_count; i++) {
        measurement_t* entry = &measurements[i];
        offset += snprintf(response + offset, JSON_BUFFER_SIZE - offset,
            "{"
            "\"bda\": \"%s\","
            "\"timestamp\": \"%lld\","
            "\"name\": \"%s\","
            "\"ble_rssi\": \"%d\","
            "\"temperature\": \"%.2f\","
            "\"humidity\": \"%.2f\","
            "\"pressure\": \"%.2f\","
            "\"acceleration_x\": \"%.3f\","
            "\"acceleration_y\": \"%.3f\","
            "\"acceleration_z\": \"%.3f\","
            "\"battery\": \"%.3f\","
            "\"tx_power\": \"%d\","
            "\"moves\": \"%d\","
            "\"sequence\": \"%d\""
            "}",
            entry->bda,
            entry->timestamp,
            entry->name,
            entry->ble_rssi,
            entry->temperature,
            entry->humidity,
            entry->pressure,
            entry->acceleration_x,
            entry->acceleration_y,
            entry->acceleration_z,
            entry->battery,
            entry->tx_power,
            entry->moves,
            entry->sequence);

        if (i < sensor_count - 1) {
            offset += snprintf(response + offset, JSON_BUFFER_SIZE - offset, ",");
        }
    }

    snprintf(response + offset, JSON_BUFFER_SIZE - offset, "]}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    const httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        const httpd_uri_t time_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = request_handler,
            .user_ctx = NULL,
        };
        httpd_register_uri_handler(server, &time_uri);
    }

    return server;
}