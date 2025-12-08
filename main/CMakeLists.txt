idf_component_register( SRCS main.c led_interface.c wifi_app.c
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_event nvs_flash esp_netif esp_wifi esp_http_server fatfs bme680
                        ) #             esp_event nvs_flash esp_netif esp_wifi esp_http_server fatfs 
