
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_log.h>

#include "measurement.h"

#define SCAN_DURATION 10

static const char *TAG = "ruuvi-esp32-bluetooth";

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x50,
    .scan_window = 0x30
  };

void ble_scan_task(void *pvParameters) {
    while (1) {
        ESP_LOGI(TAG, "(Re-)starting BLE scan...");
        ESP_ERROR_CHECK(esp_ble_gap_start_scanning(SCAN_DURATION));
        vTaskDelay(pdMS_TO_TICKS((SCAN_DURATION + 1) * 1000));
    }
}

static void ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
            ESP_LOGI(TAG, "BLE scan params set. Starting scan task...");
            xTaskCreate(ble_scan_task, "ble_scan_task", 2048, NULL, 5, NULL);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            ESP_LOGI(TAG, "BLE scan started");
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            const esp_ble_gap_cb_param_t *res = param;

            if (res->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT
                || res->scan_rst.adv_data_len == 0
                || res->scan_rst.ble_adv[5] != 0x99
                || res->scan_rst.ble_adv[6] != 0x04
                || res->scan_rst.ble_adv[7] != 0x05) {
                // Not a Ruuvi device or something else is wrong
                break;
            }

            measurement_t measurement = {};
            time(&measurement.timestamp);
            measurement.ble_rssi = res->scan_rst.rssi;

            sprintf(measurement.bda, "%02X:%02X:%02X:%02X:%02X:%02X",
                res->scan_rst.bda[0],
                res->scan_rst.bda[1],
                res->scan_rst.bda[2],
                res->scan_rst.bda[3],
                res->scan_rst.bda[4],
                res->scan_rst.bda[5]);

            const int16_t temp_raw = res->scan_rst.ble_adv[8] << 8 | res->scan_rst.ble_adv[9];
            measurement.temperature = temp_raw * 0.005f;
            const uint16_t humidity_raw = res->scan_rst.ble_adv[10] << 8 | res->scan_rst.ble_adv[11];
            measurement.humidity = humidity_raw * 0.0025f;
            const uint16_t pressure_raw = res->scan_rst.ble_adv[12] << 8 | res->scan_rst.ble_adv[13];
            measurement.pressure = (pressure_raw + 50000) * 0.01f;
            const int16_t acc_x_raw = res->scan_rst.ble_adv[14] << 8 | res->scan_rst.ble_adv[15];
            measurement.acceleration_x = acc_x_raw * 0.001f;
            const int16_t acc_y_raw = res->scan_rst.ble_adv[16] << 8 | res->scan_rst.ble_adv[17];
            measurement.acceleration_y = acc_y_raw * 0.001f;
            const int16_t acc_z_raw = res->scan_rst.ble_adv[18] << 8 | res->scan_rst.ble_adv[19];
            measurement.acceleration_z =acc_z_raw * 0.001f;
            const uint16_t bat_raw = res->scan_rst.ble_adv[20] << 3 | res->scan_rst.ble_adv[21] >> 5;
            measurement.battery = (bat_raw + 1600) * 0.001f;
            measurement.txpower = (res->scan_rst.ble_adv[21] & 0x1f) * 2 - 40;
            measurement.moves = res->scan_rst.ble_adv[22];
            measurement.sequence = res->scan_rst.ble_adv[23] << 8 | res->scan_rst.ble_adv[24];

            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "Timestamp:      %lld", measurement.timestamp);
            ESP_LOGI(TAG, "BLE RSSI:       %d", measurement.ble_rssi);
            ESP_LOGI(TAG, "Device Address: %s", measurement.bda);
            ESP_LOGI(TAG, "Device Name:    %s", measurement.name);
            ESP_LOGI(TAG, "Temperature:    %.2f C", measurement.temperature);
            ESP_LOGI(TAG, "Humidity:       %.2f %%", measurement.humidity);
            ESP_LOGI(TAG, "Pressure:       %.2f hPa", measurement.pressure);
            ESP_LOGI(TAG, "Acceleration X: %.3f G", measurement.acceleration_x);
            ESP_LOGI(TAG, "Acceleration Y: %.3f G", measurement.acceleration_y);
            ESP_LOGI(TAG, "Acceleration Z: %.3f G", measurement.acceleration_z);
            ESP_LOGI(TAG, "Battery:        %.3f V", measurement.battery);
            ESP_LOGI(TAG, "TX Power:       %d dBm", measurement.txpower);
            ESP_LOGI(TAG, "Moves:          %d", measurement.moves);
            ESP_LOGI(TAG, "Sequence:       %d", measurement.sequence);

            break;
        }
        case ESP_GAP_SEARCH_INQ_CMPL_EVT: {
            ESP_LOGI(TAG, "BLE scan stopped");
            break;
        }
        default: {
            ESP_LOGI(TAG, "Unknown event: %d", event);
            break;
        }
    }
}

esp_err_t init_bluetooth(void) {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());
    esp_ble_gap_register_callback(ble_gap_cb);
    esp_ble_gap_set_scan_params(&ble_scan_params);

    return ESP_OK;
}