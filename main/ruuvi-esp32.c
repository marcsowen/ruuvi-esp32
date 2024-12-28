
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>

#include "wifi.h"
#include "ntp.h"
#include "bluetooth.h"
#include "esp_sleep.h"
#include "http.h"

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    init_wifi();
    init_ntp();
    init_bluetooth();
    start_webserver();
}
