/**
 * @file http_server.h
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef HTTP_SERVER_H_
#define HTTP_SERVER_H_

#define OTA_UPDATE_PENDING      0
#define OTA_UPDATE_SUCCESSFUL    1
#define OTA_UPDATE_FAILED       -1

#define OTA_BUFFER_START        "\r\n\r\n"
/**
 * @brief Messages for the HTTP monitor
 * 
 */
typedef enum {
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAIL,
    HTTP_MSG_OTA_UPDATE_INITIALIZED,
    HTTP_MSG_OTA_UPDATE_SUCCESS,
    HTTP_MSG_OTA_UPDATE_FAIL,
} http_server_message_e;

/**
 * @brief structure for message queue
 * 
 */
typedef struct http_server_queue_message
{
    http_server_message_e msgID;
} http_server_queue_message_t;

/**
 * @brief Sends a message to the Queue
 * @param msgID Message ID to send
 * @return true if successful, false otherwise
 * @note expand as needed
 */
BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

/**
 * @brief Starts the HTTP server
 * 
 */
void http_server_start(void);

/**
 * @brief Stops the HTTP server
 * 
 */
void http_server_stop(void);

/**
 * @brief timer callback function which succesfully calls esp_restart upon succesful firmware update.
 * 
 */
void http_server_fw_reset_callback(void *arg);

#endif // HTTP_SERVER_H_