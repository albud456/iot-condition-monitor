/**
 * @file wifi_app.c
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
//RTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
//WIFI includes
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"
//App includes
#include "led_interface.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "http_server.h"

//For serial console messages and debugging
static const char TAG[] = "wifi_app";

//queue handle used to control main flow of events
static QueueHandle_t wifi_app_queue_handle;

//netif objects for station and access point
esp_netif_t* esp_netif_station = NULL;
esp_netif_t* esp_netif_ap = NULL;  

/**
 * @brief wifi app event handler
 * @param arg data aside from the event data that is passed to the handler
 * @param event_base the base ID of the event to register the handle for
 * @param event_id ID for the event to register the handle for
 * @param event_data 
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WIFI EVENT AP START");
                break;

            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WIFI EVENT AP STOP");
                break;

            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
                break;

            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
                break;

            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI EVENT STA START");
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI EVENT STA CONNECTED");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI EVENT STA DISCONNECTED");
                break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                break;
        }
    }
}

/**
 * @brief initializes the wifi app event handler for wifi and ip events
 * 
 */
static void wifi_app_event_handler_init(void)
{
    //event loop for wifi driver
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    //event handler for both wifi and IP events
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));
}

/**
 * @brief initializes the TCP stack and default wifi configuration
 * 
 */
static void wifi_app_default_settings_init(void)
{
    //Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    //Default wifi config -- operations must be in this order!
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    esp_netif_station = esp_netif_create_default_wifi_sta();
    esp_netif_ap = esp_netif_create_default_wifi_ap();

}
/**
  * @brief confiugures the WIFI access point settings, and assigns the static IP to the SoftAP
  * 
  */
static void wifi_app_soft_ap_config(void){
    //Soft AP configuration
    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = WIFI_AP_CHANNEL,
            .password = WIFI_AP_PASSWORD,
            .ssid_hidden = WIFI_AP_SSID_HIDDEN,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .beacon_interval = WIFI_AP_BEACON_INTERVAL,
        },
    };
    //Configure the DHCP for the AP
    esp_netif_ip_info_t ap_ip_info;
    memset(&ap_ip_info, 0x00, sizeof(ap_ip_info));
    esp_netif_dhcps_stop(esp_netif_ap);                     //stop DHCP server first, before making updates
    inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);         //assign access points static IP, GW, and netmask
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ap_ip_info)); //statically configures the network interface
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap));   //start DHCP server
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));    //setting the mode as access point/Station Mode
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config)); //setting the access point configuration
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_AP_BANDWIDTH)); //setting the access point bandwidth
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));  //setting the station power save mode
    
}

/**
 * @brief main task for the wifi app
 * 
 * @param pvParameter 
 */
static void wifi_app_task(void *pvParameter)
{
    wifi_app_queue_msg_t msg;
    //init event handler
    wifi_app_event_handler_init();

    //initialize TCP stack
    wifi_app_default_settings_init();

    //soft ap config
    wifi_app_soft_ap_config();

    //start the wifi stack (built in function)
    ESP_ERROR_CHECK(esp_wifi_start());

    //send initial message
    wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

    for(;;)
    {
        if(xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
                case WIFI_APP_MSG_START_HTTP_SERVER:
                    ESP_LOGI(TAG, "Starting HTTP Server");
                    //Start HTTP server
                    http_server_start();
                    rgb_led_http_server_started();
                    break;

                case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                    ESP_LOGI(TAG, "Connecting to new AP from HTTP Server");
                    //Connect to new AP
                    //wifi_app_connect_to_new_ap();
                    rgb_led_wifi_app_started();
                    break;

                case WIFI_APP_MSG_STATION_CONNECTED_WITH_IP:
                    ESP_LOGI(TAG, "Station connected and got IP from AP");
                    //Indicate wifi connected via LED
                    rgb_led_wifi_connected();
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown message ID received: %d", msg.msgID);
                    break;
            }
        }
    }
}

BaseType_t wifi_app_send_message(wifi_app_msg_e msgID)
{
    wifi_app_queue_msg_t msg;
    msg.msgID = msgID;

    //sends the defined message to the queue, returns result of xQueueSend
    return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);
}

void wifi_app_start(void)
{
    ESP_LOGI(TAG, "Starting WIFI APP");

    //start led
    rgb_led_wifi_app_started();

    //disable default msgs to reduce clutter
    esp_log_level_set("wifi", ESP_LOG_NONE);

    //create the message queue
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_msg_t));

    //start the wifi app task
    xTaskCreatePinnedToCore(&wifi_app_task, 
                            "wifi_app_task", 
                            WIFI_APP_TASK_STACK_SIZE, 
                            NULL, 
                            WIFI_APP_TASK_PRIORITY, 
                            NULL, 
                            WIFI_APP_TASK_CORE_ID);
}