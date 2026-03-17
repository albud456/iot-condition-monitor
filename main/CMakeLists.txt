idf_component_register( SRCS main.c led_interface.c DHT22.c wifi_app.c http_server.c app_nvs.c wifi_reset_button.c sntp_time_sync.c aws_iot.c
                        INCLUDE_DIRS "."
                        REQUIRES driver esp_event esp_timer nvs_flash esp_netif esp_wifi esp_http_server fatfs esp-aws-iot
                        PRIV_REQUIRES app_update
                        EMBED_FILES webpage/app.css webpage/app.js webpage/favicon.ico webpage/index.html webpage/jquery-3.3.1.min.js
                        )            
target_add_binary_data(${COMPONENT_TARGET} "certs/aws_root_ca_pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate_pem_crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private_pem_key" TEXT)