/**
 * @file main.c
 * @author Daniel Albu (albud456@gmail.com)
 * @brief Application entry point
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "sntp_time_sync.h"
#include "led_interface.h"
#include "nvs_flash.h"
#include "wifi_app.h"
#include "DHT22.h"
#include "wifi_reset_button.h"
#include "aws_iot.h"

static const char *TAG = "main";

void wifi_application_connected_event(void)
{
    ESP_LOGI(TAG, "Wifi application connected");
    sntp_time_sync_task_start();
    //Start IoT Task once wifi has been secured
    aws_iot_start();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Server..");

    esp_err_t ret = nvs_flash_init();
    //These errors can be resolved by flashing and retrying
   if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    } 
    ESP_ERROR_CHECK(ret);

    //Start the Wifi
    wifi_app_start();
    //wifi reset button
    wifi_reset_button_config();
    //start DHT22 task
    DHT22_task_start();
    //Set event callback for sntp
    wifi_app_set_callback(&wifi_application_connected_event);

}
