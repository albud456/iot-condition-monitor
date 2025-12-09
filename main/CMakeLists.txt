idf_component_register( SRCS main.c led_interface.c wifi_app.c http_server.c
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_event nvs_flash esp_netif esp_wifi esp_http_server fatfs bme680
                        EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js
                        )            
