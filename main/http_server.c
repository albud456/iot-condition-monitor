/**
 * @file http_server.c
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "esp_http_server.h"
#include "esp_log.h"

#include "http_server.h"
#include "wifi_app.h"
#include "tasks_common.h"


// Tag used for ESP serial console messages
static const char *TAG = "http_server";

// HTTP server handle
static httpd_handle_t http_server_handle = NULL;

// Embedded files: JQuiery index.html, app.css, app.js, favicon.ico files
extern const uint8_t jquery_3_3_1_min_js_start[]    asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]      asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]             asm("_binary_index_html_start");
extern const uint8_t index_html_end[]               asm("_binary_index_html_end");
extern const uint8_t app_css_start[]                asm("_binary_app_css_start");
extern const uint8_t app_css_end[]                  asm("_binary_app_css_end");
extern const uint8_t app_js_start[]                 asm("_binary_app_js_start");
extern const uint8_t app_js_end[]                   asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]            asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]              asm("_binary_favicon_ico_end");

/**
 * @brief JQuery JS file HTTP GET handler is requested when accessing the webpage
 * @param req HTTP request pointer for which the uri needs to be handled
 * @return ESP_OK if file is sent successfully, ESP_FAIL otherwise
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "JQuery JS file requested");

    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
    return ESP_OK;
}
/**
 * @brief sends the index.html page when requested
 * @param req HTTP request pointer for which the uri needs to be handled
 * @return ESP_OK if file is sent successfully, ESP_FAIL otherwise
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Index.html file requested");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}
/**
 * @brief sends the app.css get handler is requested when accessing the web page
 * @param req HTTP request pointer for which the uri needs to be handled
 * @return ESP_OK if file is sent successfully, ESP_FAIL otherwise
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.css file requested");

    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);
    return ESP_OK;
}
/**
 * @brief sends the app.css get handler is requested when accessing the web page
 * @param req HTTP request pointer for which the uri needs to be handled
 * @return ESP_OK if file is sent successfully, ESP_FAIL otherwise
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.js file requested");

    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
    return ESP_OK;
}
/**
 * @brief sends the .ico (icon) file when accessing the webpage
 * @param req HTTP request pointer for which the uri needs to be handled
 * @return ESP_OK if file is sent successfully, ESP_FAIL otherwise
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "favicon.ico file requested");

    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
    return ESP_OK;
}



/**
 * @brief sets up the default httpd server config
 * @returns http server instance handle if succesful, NULL otherwise
 */
static httpd_handle_t http_server_configure(void)
{
    // generate the default config
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    //TODO: create HTTP server monitor task
    //TODO: create HTTP server message queue

    //the core that the HTTP server task should run on
    config.core_id = HTTP_SERVER_TASK_CORE_ID;

    //adjust the default priority to 1 less than the wifi application task
    config.task_priority = HTTP_SERVER_TASK_PRIORITY;

    //bump up the stack size (default is 4096 bytes)
    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

    //increasre uri handler
    config.max_uri_handlers = 20;

    //increase timeout limit
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    ESP_LOGI(TAG, "Starting HTTP server on port: '%d' -- With task priority: '%d'", config.server_port, config.task_priority);

    //start the httpd server
    if(httpd_start(&http_server_handle, &config) == ESP_OK) 
    {
        ESP_LOGI(TAG, "HTTP server configure: Registered URI handlers");

        // register query js handler
        httpd_uri_t jquery_js_uri = {
            .uri       = "/jquery-3.3.1.min.js",
            .method    = HTTP_GET,
            .handler   = http_server_jquery_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(http_server_handle, &jquery_js_uri);

        // register index.html handler
        httpd_uri_t index_html = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = http_server_index_html_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(http_server_handle, &index_html);
        
        // register app.css handler
        httpd_uri_t app_css = {
            .uri       = "/app.css",
            .method    = HTTP_GET,
            .handler   = http_server_app_css_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(http_server_handle, &app_css);

        // register app.js handler
        httpd_uri_t app_js = {
            .uri       = "/app.js",
            .method    = HTTP_GET,
            .handler   = http_server_app_js_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(http_server_handle, &app_js);

        // register favicon.ico handler
        httpd_uri_t favicon_ico = {
            .uri       = "/favicon.ico",
            .method    = HTTP_GET,
            .handler   = http_server_favicon_ico_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(http_server_handle, &favicon_ico);

        return http_server_handle;
    }
    return NULL;
}

void http_server_start(void)
{
    if (http_server_handle == NULL) {
        http_server_handle = http_server_configure();
    }

}
void http_server_stop(void)
{
    if (http_server_handle) {
        httpd_stop(http_server_handle);
        http_server_handle = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}