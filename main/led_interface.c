
#include <stdio.h>
#include <stdbool.h>
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "sdkconfig.h"

#include "led_strip.h"
#include "led_interface.h"

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
static uint8_t s_led_state = 0;
static led_strip_handle_t led_strip;
static bool rmt_active_flag = false;

static const char TAG[] = "led_strip";

static void rgb_init_led_rmt(void)
{
    ESP_LOGI(TAG, "Initialized RMT connection to LED.. setting parameters");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
    
    //indicate the device has been initialized
    rmt_active_flag = true;
}

static void blink_led(void)
{
    ESP_LOGI(TAG, "Blinking LED");
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 150, 150);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

void rgb_led_wifi_app_started(void)
{
    if(!rmt_active_flag) rgb_init_led_rmt();
    led_strip_set_pixel(led_strip, 0, 255, 102, 255);
    led_strip_refresh(led_strip);
    gpio_set_level(BLINK_GPIO, 1);
}

void rgb_led_http_server_started(void)
{
    if(!rmt_active_flag) rgb_init_led_rmt();
    led_strip_set_pixel(led_strip, 0, 204, 255, 51);
    led_strip_refresh(led_strip);
    gpio_set_level(BLINK_GPIO, 1);
}

void rgb_led_wifi_connected(void)
{   
    if(!rmt_active_flag) rgb_init_led_rmt();
    led_strip_set_pixel(led_strip, 0, 0, 255, 153);
    led_strip_refresh(led_strip);
    gpio_set_level(BLINK_GPIO, 1);
}




