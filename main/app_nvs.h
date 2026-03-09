/**
 * @file app_nvs.c
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef APP_NVS_C_
#define APP_NVS_C_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief save station and wifi credentials to nvs
 * @return ESP_OK if succesful
 */
esp_err_t app_nvs_save_sta_creds(void);

/**
 * @brief load the previously saved credentials from NVS
 * @return true if previously saved credentials were found
 */
bool app_nvs_load_sta_creds(void);

/**
 * @brief clears station mode credentials from NVS
 * @return ESP_OK if succesful
 * 
 */
esp_err_t app_nvs_clear_sta_creds(void);


#endif /*APP_NVS_C_*/