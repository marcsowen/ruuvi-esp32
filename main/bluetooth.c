
#include <esp_bt.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "measurement.h"
#include "http.h"

#define LED_GPIO_PIN GPIO_NUM_2

static const char *TAG = "ruuvi-esp32-bluetooth";

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC: {
            struct ble_hs_adv_fields fields;
            int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);

            if (rc == 0 && fields.mfg_data_len > 0) {
                const uint8_t *mfg_data = fields.mfg_data;

                if (mfg_data[0] != 0x99 || mfg_data[1] != 0x04 || mfg_data[2] != 0x05) {
                    // Not a Ruuvi device or not format 5
                    break;
                }

                // Flash LED
                gpio_set_level(LED_GPIO_PIN, 1);

                measurement_t measurement = {};
                time(&measurement.timestamp);
                measurement.ble_rssi = event->disc.rssi;

                sprintf(measurement.bda, "%02X:%02X:%02X:%02X:%02X:%02X",
                    event->disc.addr.val[5], event->disc.addr.val[4], event->disc.addr.val[3],
                    event->disc.addr.val[2], event->disc.addr.val[1], event->disc.addr.val[0]);

                const int16_t temp_raw = mfg_data[3] << 8 | mfg_data[4];
                measurement.temperature = temp_raw * 0.005f;
                const uint16_t humidity_raw = mfg_data[5] << 8 | mfg_data[6];
                measurement.humidity = humidity_raw * 0.0025f;
                const uint16_t pressure_raw = mfg_data[7] << 8 | mfg_data[8];
                measurement.pressure = (pressure_raw + 50000) * 0.01f;
                const int16_t acc_x_raw = mfg_data[9] << 8 | mfg_data[10];
                measurement.acceleration_x = acc_x_raw * 0.001f;
                const int16_t acc_y_raw = mfg_data[11] << 8 | mfg_data[12];
                measurement.acceleration_y = acc_y_raw * 0.001f;
                const int16_t acc_z_raw = mfg_data[13] << 8 | mfg_data[14];
                measurement.acceleration_z =acc_z_raw * 0.001f;
                const uint16_t bat_raw = mfg_data[15] << 3 | mfg_data[16] >> 5;
                measurement.battery = (bat_raw + 1600) * 0.001f;
                measurement.tx_power = (mfg_data[16] & 0x1f) * 2 - 40;
                measurement.moves = mfg_data[17];
                measurement.sequence = mfg_data[18] << 8 | mfg_data[19];

                // ESP_LOGI(TAG, "");
                // ESP_LOGI(TAG, "Timestamp:      %lld", measurement.timestamp);
                // ESP_LOGI(TAG, "BLE RSSI:       %d", measurement.ble_rssi);
                // ESP_LOGI(TAG, "Device Address: %s", measurement.bda);
                // ESP_LOGI(TAG, "Device Name:    %s", measurement.name);
                // ESP_LOGI(TAG, "Temperature:    %.2f C", measurement.temperature);
                // ESP_LOGI(TAG, "Humidity:       %.2f %%", measurement.humidity);
                // ESP_LOGI(TAG, "Pressure:       %.2f hPa", measurement.pressure);
                // ESP_LOGI(TAG, "Acceleration X: %.3f G", measurement.acceleration_x);
                // ESP_LOGI(TAG, "Acceleration Y: %.3f G", measurement.acceleration_y);
                // ESP_LOGI(TAG, "Acceleration Z: %.3f G", measurement.acceleration_z);
                // ESP_LOGI(TAG, "Battery:        %.3f V", measurement.battery);
                // ESP_LOGI(TAG, "TX Power:       %d dBm", measurement.txpower);
                // ESP_LOGI(TAG, "Moves:          %d", measurement.moves);
                // ESP_LOGI(TAG, "Sequence:       %d", measurement.sequence);

                update_sensor(measurement);

                vTaskDelay(pdMS_TO_TICKS(50));
                gpio_set_level(LED_GPIO_PIN, 0);
            }
            break;
        }
        case BLE_GAP_EVENT_DISC_COMPLETE: {
            ESP_LOGI(TAG, "BLE scan completed.");
            break;
        }
        default: {
            ESP_LOGI(TAG, "Unknown event: %d", event->type);
            break;
        }
    }

    return 0;
}

void start_scan() {
    struct ble_gap_disc_params scan_params = {
        .itvl = 0x50,
        .window = 0x30,
        .filter_policy = BLE_HCI_SCAN_FILT_NO_WL,
        .limited = 0,
        .passive = 1
    };

    ESP_LOGI(TAG, "Starting BLE scan...");
    const int rc = ble_gap_disc(0, BLE_HS_FOREVER, &scan_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting BLE scan: %d", rc);
    }
}

void ble_host_task(void *param) {
    nimble_port_run();
}

esp_err_t init_bluetooth(void) {
    gpio_reset_pin(LED_GPIO_PIN);
    gpio_set_direction(LED_GPIO_PIN, GPIO_MODE_OUTPUT);

    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.reset_cb = NULL;
    ble_hs_cfg.sync_cb = start_scan;
    nimble_port_freertos_init(ble_host_task);

    return ESP_OK;
}