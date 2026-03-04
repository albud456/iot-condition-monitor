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
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "sys/param.h"
#include <inttypes.h>

#include <DHT22.h>
#include "http_server.h"
#include "wifi_app.h"
#include "tasks_common.h"


//Tag used for ESP serial console messages
static const char *TAG = "http_server";

//firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

//HTTP server handle
static httpd_handle_t http_server_handle = NULL;

//HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

//HTTP queue handle for main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

/**
 * @brief ESP32 timer config passed to esp_timer_create
 * 
 */
const esp_timer_create_args_t fw_update_reset_args = {
    .callback = &http_server_fw_reset_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

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
 * @brief Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true
 * 
 */
 static void http_server_fw_update_reset_timer(void)
 {
    if(g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
    {
        ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update succesful starting FW update reset timer");

        //give the webpage a chance to recieve an aknowledge back and initialize the timer
        ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
        ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
    }
    else
    {
        ESP_LOGI(TAG, "http_server_fw_update_reset_timer: Firmware update unsuccessful");
    }
 }
/**
 * @brief http server monitor task used to track events of the http server
 * @param pvParameters parameter which can be passed to the task
 */
static void http_server_monitor(void *parameter)
{
    http_server_queue_message_t msg;

    for(;;){
        if(xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
        {
            switch(msg.msgID)
            {
                case HTTP_MSG_WIFI_CONNECT_INIT:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
                    break;
                case HTTP_MSG_WIFI_CONNECT_SUCCESS:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
                    break;
                case  HTTP_MSG_WIFI_CONNECT_FAIL:
                    ESP_LOGI(TAG, " HTTP_MSG_WIFI_CONNECT_FAIL");
                    break;
                case HTTP_MSG_OTA_UPDATE_SUCCESS:
                    ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESS");
                    g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
                    http_server_fw_update_reset_timer();
                    break;
                case HTTP_MSG_OTA_UPDATE_FAIL:
                    ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAIL");
                    g_fw_update_status = OTA_UPDATE_FAILED;
                    break;
                case HTTP_MSG_OTA_UPDATE_INITIALIZED:
                    ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_INITIALIZED");
                    break;
                default:
                    break;
            }
            
        }
    }

}
 
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
 * @brief recieves the .bin file via the webpage and handles the firmware update
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK, otherwise ESP_FAIL if timeout occurs and the update cannot be initiated
 */
 esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
 {
    esp_ota_handle_t ota_handle;

    char ota_buff[1024]; //may need to be expanded
    int content_length = req->content_len;
    int content_recieved = 0;
    int recv_len;
    bool is_req_body_started = false;
    bool flash_succesful = false;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    do
    {
        if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
        {
            //Check timeout occured
            if(recv_len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
                continue; //retry recieving if timeout occurs
            }
            ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other error %d", recv_len);
            return ESP_FAIL;
        }
        printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_recieved, content_length);

        //check if initial block of data, if so, it will have the information in the header
        if(!is_req_body_started)
        {
            is_req_body_started = true;
            //get the location of the .bin file content (remove the web from data)
            char *body_start_p = strstr(ota_buff, OTA_BUFFER_START) + strlen(OTA_BUFFER_START);
            int body_part_len = recv_len - (body_start_p - ota_buff);
            printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

            esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
            if(err != ESP_OK)
            {
                printf("http_server_OTA_update_handler: Error with OTA, cancelling OTA\r\n");
                return ESP_FAIL;
            }
            else
            {
                printf("http_server_OTA_update_handler: writing to partition subtype %d at offset 0x%lx\r\n", update_partition->subtype, update_partition->address);
            }
            //write first part of data
            esp_ota_write(ota_handle, body_start_p, body_part_len);
            content_recieved += body_part_len;
        }
        else
        {
            //write OTA data
            esp_ota_write(ota_handle, ota_buff, recv_len);
            content_recieved += recv_len;
        }
    }while(recv_len > 0 && content_recieved < content_length);

    if(esp_ota_end(ota_handle) == ESP_OK)
    {
        //update partition
        if(esp_ota_set_boot_partition(update_partition) == ESP_OK)
        {
            const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
            flash_succesful = true;
        }
        else
        {
            ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASH ERROR");
        }
    }
    else{
        ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR");
    }
    if(flash_succesful) 
    { 
        http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESS);
    } else {http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAIL);}
    
    return ESP_OK;
}
/**
 * @brief OTA status handler responds with the firmware update status after the OTA is started
 * also gives compile time, date, when page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
    char otaJSON[100];
    ESP_LOGI(TAG,"OTA status requested");
    sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}",
        g_fw_update_status, __TIME__, __DATE__);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, otaJSON, strlen(otaJSON));

    return ESP_OK;
}
/**
 * @brief dht sensor reading JSON handler responds with DHT22 sensor data
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
static esp_err_t http_server_get_dht_sensor_readings_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/dhtSensor.json requested");

    char dhtSensorJSON[100];
    sprintf(dhtSensorJSON, "{\"temp\":\"%.1f\",\"humidity\":\"%.1f\"}", getTemperature(), getHumidity());
    //sprintf(dhtSensorJSON, "{\"temp\":\"0.25\",\"humidity\":\"0.44\"}");
    
    // Set headers: JSON content, no caching, allow CORS
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    esp_err_t err = httpd_resp_send(req, dhtSensorJSON, strlen(dhtSensorJSON));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send /dhtSensor.json response");
    }

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

    //Create HTTP server monitor task
    xTaskCreatePinnedToCore(&http_server_monitor, 
                            "http_server_monitor", 
                            HTTP_SERVER_MONITOR_STACK_SIZE,
                            NULL,
                            HTTP_SERVER_MONITOR_PRIORITY,
                            &task_http_server_monitor,
                            HTTP_SERVER_MONITOR_CORE_ID
                            );

    //TODO: create HTTP server message queue
    http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));
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
        
        //register OTA update handler
        httpd_uri_t OTA_update = {
            .uri = "/OTAupdate",
            .method = HTTP_POST,
            .handler = http_server_OTA_update_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &OTA_update);

        //register OTA status handler
        httpd_uri_t OTA_status = {
            .uri = "/OTAstatus",
            .method = HTTP_POST,
            .handler = http_server_OTA_status_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &OTA_status);

        //register dhtSensor.json handler
        httpd_uri_t dht_sensor_json = {
            .uri = "/dhtSensor.json",
            .method = HTTP_GET,
            .handler = http_server_get_dht_sensor_readings_json_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &dht_sensor_json);
        ESP_LOGI(TAG, "Registered /dhtSensor.json handler");

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
    if(task_http_server_monitor){
        vTaskDelete(task_http_server_monitor);
        ESP_LOGI(TAG, "HTTP server stop: stopping HTTP server monitor");
        task_http_server_monitor = NULL;
    }
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
    http_server_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_reset_callback(void *arg)
{
    ESP_LOGI(TAG, "http_server_fw_update_reset_callback: timed-out, restarting device");
    esp_restart();
}