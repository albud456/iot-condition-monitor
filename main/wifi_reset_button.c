
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "tasks_common.h"
#include "wifi_app.h"
#include "wifi_reset_button.h"

static const char TAG[] = "wifi_reset_button";

//semaphore handle
SemaphoreHandle_t wifi_reset_sem = NULL;

/**
 * @brief ISR handle for the wifi reset boot button
 * @param arg parameter which can be passed to the ISR handler
 */
 void IRAM_ATTR wifi_reset_button_isr_handler(void *arg)
 {
    //notify the button task
    xSemaphoreGiveFromISR(wifi_reset_sem, NULL);
 }
/**
 * @brief wifi reset button task reacts to a BOOT button event by 
 * sending a msg to the wifi app to disconnect from the wifi and clear nvs
 * @param pvParam parameter which can be passed to the tank
 */
static void wifi_reset_button_task(void *pvParam)
{
    for(;;)
    {
        if(xSemaphoreTake(wifi_reset_sem, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "wifi_reset_button_task: Interrupt occured");

            //send msg to disconnect wifi & clear creds
            wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);

            //add delay to prevent double bounce and user button mashing
            vTaskDelay(2000/portTICK_PERIOD_MS);
        }
    }
}

void wifi_reset_button_config(void)
{
    //create semaphore
    wifi_reset_sem = xSemaphoreCreateBinary();

    //configure hardware
    esp_rom_gpio_pad_select_gpio(WIFI_RESET_BUTTON);
    gpio_set_direction(WIFI_RESET_BUTTON, GPIO_MODE_INPUT);

    //enable interrupt, falling edge
    gpio_set_intr_type(WIFI_RESET_BUTTON, GPIO_INTR_NEGEDGE);

    //create the task
    xTaskCreatePinnedToCore(&wifi_reset_button_task, 
                            "wifi_reset_button", 
                            WIFI_RESET_BUTTON_TASK_STACK_SIZE,
                            NULL,
                            WIFI_RESET_BUTTON_TASK_PRIORITY,
                            NULL,
                            WIFI_RESET_BUTTON_TASK_CORE_ID
                            );
    //install gpio ISR
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //attach ISR
    gpio_isr_handler_add(WIFI_RESET_BUTTON, wifi_reset_button_isr_handler, NULL);

}