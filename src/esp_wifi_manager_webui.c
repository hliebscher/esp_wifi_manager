/**
 * @file esp_wifi_manager_webui.c
 * @brief Embedded Web UI serving for WiFi Manager
 */

#include "esp_wifi_manager_priv.h"
#include "esp_log.h"
#include <string.h>
#include <sys/stat.h>

#ifdef CONFIG_WIFI_MGR_ENABLE_WEBUI

static const char *TAG = "wifi_mgr_webui";

// Embedded files (linked via CMakeLists.txt EMBED_FILES)
// Symbol names: _binary_{filename with . replaced by _}_{start|end}
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t app_js_gz_start[] asm("_binary_app_js_gz_start");
extern const uint8_t app_js_gz_end[] asm("_binary_app_js_gz_end");
extern const uint8_t index_css_gz_start[] asm("_binary_index_css_gz_start");
extern const uint8_t index_css_gz_end[] asm("_binary_index_css_gz_end");

/**
 * @brief Try to serve file from custom path (SPIFFS/LittleFS)
 */
static bool serve_from_filesystem(httpd_req_t *req, const char *filepath)
{
#ifdef CONFIG_WIFI_MGR_WEBUI_CUSTOM_PATH
    const char *base_path = CONFIG_WIFI_MGR_WEBUI_CUSTOM_PATH;
    if (!base_path || !base_path[0]) {
        return false;
    }

    char fullpath[128];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_path, filepath);

    // Check if file exists
    struct stat st;
    if (stat(fullpath, &st) != 0) {
        return false;
    }

    FILE *f = fopen(fullpath, "r");
    if (!f) {
        return false;
    }

    // Determine content type
    const char *content_type = "text/plain";
    if (strstr(filepath, ".html")) {
        content_type = "text/html";
    } else if (strstr(filepath, ".js")) {
        content_type = "application/javascript";
    } else if (strstr(filepath, ".css")) {
        content_type = "text/css";
    } else if (strstr(filepath, ".json")) {
        content_type = "application/json";
    } else if (strstr(filepath, ".png")) {
        content_type = "image/png";
    } else if (strstr(filepath, ".svg")) {
        content_type = "image/svg+xml";
    }

    httpd_resp_set_type(req, content_type);

    // Check for gzipped version
    char gzpath[132];
    snprintf(gzpath, sizeof(gzpath), "%s.gz", fullpath);
    if (stat(gzpath, &st) == 0) {
        fclose(f);
        f = fopen(gzpath, "r");
        if (f) {
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        }
    }

    // Stream file
    char buf[512];
    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, read_bytes);
    }
    httpd_resp_send_chunk(req, NULL, 0);

    fclose(f);
    ESP_LOGD(TAG, "Served from filesystem: %s", filepath);
    return true;
#else
    return false;
#endif
}

/**
 * @brief Handler for index.html (root path)
 */
static esp_err_t handler_webui_index(httpd_req_t *req)
{
    // Try filesystem first
    if (serve_from_filesystem(req, "/index.html")) {
        return ESP_OK;
    }

    // Serve embedded index.html
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start,
                    index_html_end - index_html_start);
    return ESP_OK;
}

/**
 * @brief Handler for app.js
 */
static esp_err_t handler_webui_app_js(httpd_req_t *req)
{
    // Try filesystem first
    if (serve_from_filesystem(req, "/assets/app.js")) {
        return ESP_OK;
    }

    // Serve embedded gzipped file
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000");
    httpd_resp_send(req, (const char *)app_js_gz_start,
                    app_js_gz_end - app_js_gz_start);
    return ESP_OK;
}

/**
 * @brief Handler for index.css
 */
static esp_err_t handler_webui_index_css(httpd_req_t *req)
{
    // Try filesystem first
    if (serve_from_filesystem(req, "/assets/index.css")) {
        return ESP_OK;
    }

    // Serve embedded gzipped file
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000");
    httpd_resp_send(req, (const char *)index_css_gz_start,
                    index_css_gz_end - index_css_gz_start);
    return ESP_OK;
}

/**
 * @brief Initialize Web UI handlers
 */
esp_err_t wifi_mgr_webui_init(httpd_handle_t httpd)
{
    if (!httpd) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing Web UI");

    // Register handlers
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handler_webui_index,
    };
    httpd_register_uri_handler(httpd, &index_uri);

    httpd_uri_t app_js_uri = {
        .uri = "/assets/app.js",
        .method = HTTP_GET,
        .handler = handler_webui_app_js,
    };
    httpd_register_uri_handler(httpd, &app_js_uri);

    httpd_uri_t css_uri = {
        .uri = "/assets/index.css",
        .method = HTTP_GET,
        .handler = handler_webui_index_css,
    };
    httpd_register_uri_handler(httpd, &css_uri);

    size_t total_size = (index_html_end - index_html_start) +
                        (app_js_gz_end - app_js_gz_start) +
                        (index_css_gz_end - index_css_gz_start);
    ESP_LOGI(TAG, "Web UI registered (embedded size: %zu bytes)", total_size);

    return ESP_OK;
}

#else

esp_err_t wifi_mgr_webui_init(httpd_handle_t httpd)
{
    (void)httpd;
    return ESP_OK;
}

#endif // CONFIG_WIFI_MGR_ENABLE_WEBUI
