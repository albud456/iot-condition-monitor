/**
 * @file wifi_app.h
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef WIFI_APP_H_
#define WIFI_APP_H_

#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"

//Callback typedef
typedef void (*wifi_connected_event_callback_t)(void);

//Wifi app settings
#define WIFI_AP_SSID            "IOT_TEMP_MONITOR"      //access point name
#define WIFI_AP_PASSWORD        "123456789"             //password to connect
#define WIFI_AP_CHANNEL         1                       //ap channel used
#define WIFI_AP_SSID_HIDDEN     0                       //0 - hidden, 1 - visible
#define WIFI_AP_MAX_CONNECTIONS 2                       //max num devices that can connect
#define WIFI_AP_BEACON_INTERVAL 100                     //beacon interval in ms
#define WIFI_AP_IP              "192.168.0.1"           //AP IP address
#define WIFI_AP_GATEWAY         "192.168.0.1"           //AP Gateway
#define WIFI_AP_NETMASK         "255.255.255.0"         //AP Netmask
#define WIFI_AP_BANDWIDTH       WIFI_BW_HT20            //AP Bandwidth
#define WIFI_STA_POWER_SAVE     WIFI_PS_NONE            //STA Power save mode
#define MAXIMUM_SSID_LENGTH     32                      //Maximum SSID length (IEEE Standard is 32)
#define MAXIMUM_PASSWORD_LENGTH 64                      //Maximum Password length (IEEE Standard is 64)
#define MAXIMUM_CONNECTION_RETRIES  5  

//define netif objects for station and access point
extern esp_netif_t* esp_netif_station;
extern esp_netif_t* esp_netif_ap;

/**
 * @brief Message ID's for the WIFI application task
 * @note expand as needed
 */
typedef enum wifi_app_message
{
    WIFI_APP_MSG_START_HTTP_SERVER = 0,
    WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
    WIFI_APP_MSG_STA_CONNECTED_WITH_IP,
    WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
    WIFI_APP_MSG_LOAD_SAVED_CREDENTIALS,
    WIFI_APP_MSG_STA_DISCONNECTED
} wifi_app_msg_e;

/**
 * @brief structure for message queue
 * @note expand for future messages
 */
typedef struct wifi_app_queue_message
{
    wifi_app_msg_e msgID;
} wifi_app_queue_msg_t;

/**
 * @brief sends msg to queue
 * @param msgID message id of type wifi_app_msg_e
 * @return pdTRUE if a msg was succesfully sent to queue, otherwise pdFALSE
 */
BaseType_t wifi_app_send_message(wifi_app_msg_e msgID);

/**
 * @brief starts wifi RTOS task
 * 
 */
void wifi_app_start(void);

/**
 * @brief gets the wifi configuration
 * 
 */
wifi_config_t* wifi_app_get_wifi_config(void);

/**
 * @brief sets the callback function
 * 
 */
void wifi_app_set_callback(wifi_connected_event_callback_t cb);

/**
 * @brief calls the callback function
 * 
 */
void wifi_app_trigger_callback(void);

#endif /*WIFI_APP_H_*/