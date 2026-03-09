/**
 * @file app_nvs.c
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-09
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <nvs_flash.h>

#include <wifi_app.h>
#include "app_nvs.h"


static const char *TAG = "app_nvs";

//nvs namespace used for sta credentials
const char app_nvs_sta_creds_namespace[] = "stacreds";

esp_err_t app_nvs_save_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_save_sta_credentials: Saving STA creds to Flash");

    wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();

    if(wifi_sta_config)
    {
        esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: ERROR %s opening nvs handle\n", esp_err_to_name(esp_err));
            return esp_err;
        }

        //set SSID
        esp_err = nvs_set_blob(handle, "ssid", wifi_sta_config->sta.ssid, MAXIMUM_SSID_LENGTH);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: ERROR %s setting ssid to nvs\n", esp_err_to_name(esp_err));
            return esp_err;
        }
        //set PASS
        esp_err = nvs_set_blob(handle, "password", wifi_sta_config->sta.password, MAXIMUM_SSID_LENGTH);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: ERROR %s setting password to nvs\n", esp_err_to_name(esp_err));
            return esp_err;
        }

        //commit credentials to nvs
        esp_err = nvs_commit(handle);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: ERROR %s committing credentials to nvs\n", esp_err_to_name(esp_err));
            return esp_err;
        }
        nvs_close(handle);
        ESP_LOGI(TAG, "app_nvs_save_sta_credentials: wrote wifi_sta_config: Station SSID: %s Password: %s", wifi_sta_config->sta.ssid,wifi_sta_config->sta.password);
    }
    return ESP_OK;
}

bool app_nvs_load_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_load_sta_creds: loading sta creds from flash");

    if(nvs_open(app_nvs_sta_creds_namespace, NVS_READONLY, &handle) == ESP_OK)
    {
        wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();
        if(wifi_sta_config == NULL)
        {
            wifi_sta_config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
        }
        memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));
        size_t wifi_config_size = sizeof(wifi_config_t);
        uint8_t *wifi_config_buff = (uint8_t*)malloc(sizeof(uint8_t) * wifi_config_size);
        //memset(wifi_config_buff, 0x00, sizeof(wifi_config_buff));
        memset(wifi_config_buff, 0x00, wifi_config_size);

        //load ssid
        wifi_config_size = sizeof(wifi_sta_config->sta.ssid);
        esp_err = nvs_get_blob(handle, "ssid", wifi_config_buff, &wifi_config_size);
        if(esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            printf("app_nvs_load_sta_creds: %s no station SSID found in NVS\n", esp_err_to_name(esp_err));
            return false;
        }
        memcpy(wifi_sta_config->sta.ssid, wifi_config_buff, wifi_config_size);

        //load password
        wifi_config_size = sizeof(wifi_sta_config->sta.password);
        esp_err = nvs_get_blob(handle, "password", wifi_config_buff, &wifi_config_size);
        if(esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            printf("app_nvs_load_sta_creds: %s no station password found in NVS\n", esp_err_to_name(esp_err));
            return false;
        }
        memcpy(wifi_sta_config->sta.password, wifi_config_buff, wifi_config_size);

        free(wifi_config_buff);
        nvs_close(handle);
        printf("app_nvs_load_sta_creds: found SSID: %s and Password %s", wifi_sta_config->sta.ssid, wifi_sta_config->sta.password);
        return wifi_sta_config->sta.ssid[0] != '\0';
    }
    else{return false;}

}


esp_err_t app_nvs_clear_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_clear_sta_creds: clearing sta credentials from flash");
    
    esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
    if(esp_err != ESP_OK)
    {
        printf("app_nvs_clear_sta_creds: %s error opening nvs handle", esp_err_to_name(esp_err));
        return esp_err;
    }
    //erase specific key credentials. may add data to NVS later on, do not wipe all.
    else
    {
        esp_err = nvs_erase_key(handle, "ssid");
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_clear_sta_creds: %s unable to delete ssid in NVS -- Check for corrupted NVS partition\n", esp_err_to_name(esp_err));
            return esp_err;
        }
        esp_err = nvs_erase_key(handle, "password");
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_clear_sta_creds: %s unable to delete ssid in NVS -- Check for corrupted NVS partition\n", esp_err_to_name(esp_err));
            return esp_err;
        }
    }
    //commit changes to nvs
    esp_err = nvs_commit(handle);
    if(esp_err != ESP_OK)
    {
        printf("app_nvs_clear_sta_creds: %s Something went wrong with NVS commit!\n", esp_err_to_name(esp_err));
        return esp_err;
    }
    nvs_close(handle);
    printf("app_nvs_clear_sta_creds: clearned NVS credentials!\n");

    return ESP_OK;
}
