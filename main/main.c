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

#include "led_interface.h"
#include "nvs_flash.h"
#include "wifi_app.h"

static const char *TAG = "temp-detect";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    //these errors can be resolved by flashing and retrying
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }   
    ESP_ERROR_CHECK(ret);

    //Start the Wifi
    wifi_app_start();
    rgb_led_wifi_app_started();

}