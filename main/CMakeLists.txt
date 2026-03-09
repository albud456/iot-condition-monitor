idf_component_register( SRCS main.c led_interface.c DHT22.c wifi_app.c http_server.c app_nvs.c wifi_reset_button.c
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_event esp_timer nvs_flash esp_netif esp_wifi esp_http_server fatfs
                        PRIV_REQUIRES app_update
                        EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js
                        )            
