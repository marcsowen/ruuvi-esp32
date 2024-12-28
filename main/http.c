
#include <esp_http_server.h>
#include <time.h>

static esp_err_t request_handler(httpd_req_t *req) {
    time_t now;
    time(&now);

    char response[256];
    snprintf(response, sizeof(response), "{\"timestamp\": \"%lld\"}", now);

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