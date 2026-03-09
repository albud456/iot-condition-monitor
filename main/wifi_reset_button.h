/**
 * @file wifi_reset_button.h
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef WIFI_RESET_BUTTON_H_
#define WIFI_RESET_BUTTON_H_

//default interrupt flug
#define ESP_INTR_FLAG_DEFAULT   0

//wifi reset button maps to boot button on dev kit
#define WIFI_RESET_BUTTON       9

/**
 * @brief configures wifi reset button and iterrupt configuration
 * 
 */
void wifi_reset_button_config(void);

#endif