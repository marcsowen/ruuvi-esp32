
#include <esp_log.h>
#include <esp_sntp.h>

static const char *TAG = "ruuvi-esp32-ntp";

void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time sync completed");
}

void init_ntp(void) {
    ESP_LOGI(TAG, "Initializing NTP...");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "fritz.box");
    esp_sntp_setservername(1, "de.pool.ntp.org");

    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);

    esp_sntp_init();
}