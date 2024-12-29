
#include <esp_http_server.h>
#include <time.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include "measurement.h"

#define MAX_SENSORS 10
#define JSON_BUFFER_SIZE 1024

static const char *TAG = "ruuvi-esp32-http";

measurement_t measurements[MAX_SENSORS];
int sensor_count = 0;
char esp_id[18];

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

    char bssid[18];
    int8_t wifi_rssi = -1;

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        sprintf(bssid, "%02X:%02X:%02X:%02X:%02X:%02X",
            ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
            ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
        wifi_rssi = ap_info.rssi;
    }

    offset += snprintf(response + offset, JSON_BUFFER_SIZE - offset,
        "{"
        "\"esp_mac\": \"%s\","
        "\"timestamp\": %lld,"
        "\"bssid\": \"%s\","
        "\"wifi_rssi\": %d,"
        " \"sensors\": [",
        esp_id,
        time(NULL),
        bssid,
        wifi_rssi
        );

    for (int i = 0; i < sensor_count; i++) {
        measurement_t* entry = &measurements[i];
        offset += snprintf(response + offset, JSON_BUFFER_SIZE - offset,
            "{"
            "\"ble_mac\": \"%s\","
            "\"timestamp\": %lld,"
            "\"name\": \"%s\","
            "\"ble_rssi\": %d,"
            "\"temperature\": %.2f,"
            "\"humidity\": %.2f,"
            "\"pressure\": %.2f,"
            "\"acceleration_x\": %.3f,"
            "\"acceleration_y\": %.3f,"
            "\"acceleration_z\": %.3f,"
            "\"battery\": %.3f,"
            "\"tx_power\": %d,"
            "\"moves\": %d,"
            "\"sequence\": %d"
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

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));
    sprintf(esp_id, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

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