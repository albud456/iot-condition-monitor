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
#include "lwip/netdb.h"
//App includes
#include "led_interface.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "http_server.h"
#include "app_nvs.h"

//For serial console messages and debugging
static const char TAG[] = "wifi_app";

//Used for returning wifi config
wifi_config_t *wifi_config = NULL;

//Used to track the number of retries when a connection fails
static int g_retry_number;

//wifi app event group handle and status bits. Signals if we are connecting from saved creds or http server input
static EventGroupHandle_t wifi_app_event_group;
const int WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT     = BIT0;
const int WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT      = BIT1;
const int WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT    = BIT2;
const int WIFI_APP_STA_CONNECTED_GOT_IP_BIT             = BIT3;

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

                wifi_event_sta_disconnected_t *wifi_event_sta_disconnected = 
                    (wifi_event_sta_disconnected_t*)malloc(sizeof(wifi_event_sta_disconnected_t));
                *wifi_event_sta_disconnected = *((wifi_event_sta_disconnected_t*)event_data);
                printf("WIFI_EVENT_STA_DISCONNECTED, reason code %d\n", wifi_event_sta_disconnected->reason);

                if(g_retry_number < MAXIMUM_CONNECTION_RETRIES)
                {
                    esp_wifi_connect();
                    g_retry_number++;
                }
                else
                {
                    wifi_app_send_message(WIFI_APP_MSG_STA_DISCONNECTED);
                }

                break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_WITH_IP);
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
 * @brief connect the esp to an external access point (AP) using the updated station config
 * 
 */
 static void wifi_app_connect_sta(void)
 {
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_app_get_wifi_config()));
    ESP_ERROR_CHECK(esp_wifi_connect());
 }
/**
 * @brief main task for the wifi app
 * 
 * @param pvParameter 
 */
static void wifi_app_task(void *pvParameter)
{
    wifi_app_queue_msg_t msg;
    EventBits_t eventBits;

    //init event handler
    wifi_app_event_handler_init();

    //initialize TCP stack
    wifi_app_default_settings_init();

    //soft ap config
    wifi_app_soft_ap_config();

    //start the wifi stack (built in function)
    ESP_ERROR_CHECK(esp_wifi_start());

    //send initial message
    wifi_app_send_message(WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS);

    for(;;)
    {
        if(xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch (msg.msgID)
            {
                case WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS");
                    //Start HTTP server
                    if(app_nvs_load_sta_creds())
                    {
                        wifi_app_connect_sta();
                        xEventGroupSetBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
                        ESP_LOGI(TAG, "Loaded saved station credentials");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Unable to load station configuration");
                    }
                    wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

                    break;

                case WIFI_APP_MSG_START_HTTP_SERVER:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_START_HTTP_SERVER");
                    
                    //Start HTTP server
                    http_server_start();
                    rgb_led_http_server_started();

                    break;

                case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
                    //set event bit signaling user input credentials from web server
                    xEventGroupSetBits(wifi_app_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
                    //attempt a connection to new AP
                    wifi_app_connect_sta();
                    //set number of retries to 0
                    g_retry_number = 0;
                    //let the http server know about the connection attempt
                    http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_INIT);
                    //rgb_led_wifi_app_started();

                    break;

                case WIFI_APP_MSG_STA_CONNECTED_WITH_IP:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_STA_CONNECTED_WITH_IP");

                    xEventGroupSetBits(wifi_app_event_group, WIFI_APP_STA_CONNECTED_GOT_IP_BIT);
                    http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_SUCCESS);

                    eventBits = xEventGroupGetBits(wifi_app_event_group);
                    if(eventBits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT) // save STA creds if connect success (not from nvs)
                    {
                        //clear bits incase we disconnect and start again
                        xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
                    }
                    else
                    {
                        app_nvs_save_sta_creds();
                    }
                    if(eventBits & WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT)
                    {
                        xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
                    }
                    //Indicate wifi connected via LED
                    rgb_led_wifi_connected();

                    break;

                case WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT");
                    
                    eventBits = xEventGroupGetBits(wifi_app_event_group);
                    if(eventBits & WIFI_APP_STA_CONNECTED_GOT_IP_BIT)
                    {
                        xEventGroupSetBits(wifi_app_event_group, WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT);
                        g_retry_number = MAXIMUM_CONNECTION_RETRIES;
                        ESP_ERROR_CHECK(esp_wifi_disconnect());
                        app_nvs_clear_sta_creds();
                        rgb_led_http_server_started();                        
                    }

                    break;

                case WIFI_APP_MSG_STA_DISCONNECTED:
                    ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED");

                    eventBits = xEventGroupGetBits(wifi_app_event_group);
                    if(eventBits & WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT)
                    {
                        ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: ATTEMPT USING SAVED CREDENTIALS");
                        xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_USING_SAVED_CREDS_BIT);
                        app_nvs_clear_sta_creds();
                    }
                    else if (eventBits & WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT)
                    {
                        ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: ATTEMPT FROM HTTP SERVER INPUT");
                        xEventGroupClearBits(wifi_app_event_group, WIFI_APP_CONNECTING_FROM_HTTP_SERVER_BIT);
                        http_server_monitor_send_message(HTTP_MSG_WIFI_CONNECT_FAIL);
                    }
                    else if (eventBits & WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT)
                    {
                        ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: USER REQUESTED DISCONNECT");
                        xEventGroupClearBits(wifi_app_event_group, WIFI_APP_USER_REQUESTED_STA_DISCONNECT_BIT);
                        http_server_monitor_send_message(HTTP_MSG_WIFI_USER_DISCONNECT);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "WIFI_APP_MSG_STA_DISCONNECTED: ATTEMPT FAILED, AP NOT AVAILABLE");
                        //TODO: add trying to reconnect..
                    }
                    if(eventBits & WIFI_APP_STA_CONNECTED_GOT_IP_BIT)
                    {
                        xEventGroupClearBits(wifi_app_event_group, WIFI_APP_STA_CONNECTED_GOT_IP_BIT);
                    }
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

wifi_config_t* wifi_app_get_wifi_config(void)
{
    return wifi_config;
}

void wifi_app_start(void)
{
    ESP_LOGI(TAG, "Starting WIFI APP");

    //start led
    rgb_led_wifi_app_started();

    //disable default msgs to reduce clutter
    esp_log_level_set("wifi", ESP_LOG_NONE);

    //allocate memory for the wifi config
    wifi_config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
    memset(wifi_config, 0x00, sizeof(wifi_config_t));

    //create the message queue
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_msg_t));

    //create wifi application event group
    wifi_app_event_group = xEventGroupCreate();

    //start the wifi app task
    xTaskCreatePinnedToCore(&wifi_app_task, 
                            "wifi_app_task", 
                            WIFI_APP_TASK_STACK_SIZE, 
                            NULL, 
                            WIFI_APP_TASK_PRIORITY, 
                            NULL, 
                            WIFI_APP_TASK_CORE_ID);
}