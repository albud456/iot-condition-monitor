/**
 * @file led_interface.h
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef LED_INTERFACE_H_
#define LED_INTERFACE_H_


#define BLINK_GPIO 8

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    int mode;
    int timer_index;

} rgb_led_t;

static void blink_led(void);

/**
 * @brief sets LED on with unique color indicate wifi has started
 * 
 */
void rgb_led_wifi_app_started(void);

/**
 * @brief sets LED on with unique color indicate http server has started
 * 
 */
void rgb_led_http_server_started(void);

/**
 * @brief sets LED on with unique color indicate ESP is connected to an access point
 * 
 */
void rgb_led_wifi_connected(void);



#endif