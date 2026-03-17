
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"

#include "sntp_time_sync.h"
#include "tasks_common.h"
#include "http_server.h"
#include "wifi_app.h"

static const char TAG[] = "sntp_time_sync";

//SNTP opp mode set status
static bool sntp_op_mode_set = false;

/**
 * @brief initialize SNTP service using SNTP_OPMODE_POLL mode.
 * 
 */
static void sntp_time_sync_init_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP service");

    if(!sntp_op_mode_set)
    {
        //set the op mode
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_op_mode_set = true;
    }
    sntp_setservername(0, "pool.ntp.org");

    //initialize the server
    sntp_init();

    //let the http server know
    http_server_monitor_send_message(HTTP_MSG_TIME_SYNC_SERVICE_INITIALIZED);
}


/**
 * @brief gets the current time and if its not up to date,
 * the sntp_time_sync_init_sntp function is called
 * 
 */
 static void sntp_time_sync_obtain_time(void)
 {
    time_t now = 0;
    struct tm time_info = {0};

    time(&now);
    localtime_r(&now, &time_info);
    //check time, incase we need to re-init
    if(time_info.tm_year < (2026-1900))
    {
        sntp_time_sync_init_sntp();
        //set timezone
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset();
    }
 }

/**
 * @brief sntp time sync task
 * @param arg pvParam
 * 
 */
static void sntp_time_sync(void *pvParam)
{
    for(;;)
    {
        sntp_time_sync_obtain_time();
        //edit this as needed, currently rechecks time sync every 10s.
        vTaskDelay(10000/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

char* sntp_time_sync_get_time(void)
{
    static char time_buffer[100] = {0};

    time_t now = 0;
    struct tm time_info = {0};
    time(&now);
    localtime_r(&now, &time_info);
    if(time_info.tm_year < (2026-1900))
    {
        ESP_LOGI(TAG, "Time has not been initialized yet");
    }
    else
    {
        strftime(time_buffer, sizeof(time_buffer), "%d.%m.%Y %H:%M:%S", &time_info);
        ESP_LOGI(TAG, "Current time info: %s", time_buffer);
    }
    return time_buffer;
}

void sntp_time_sync_task_start(void)
{
    xTaskCreatePinnedToCore(&sntp_time_sync, 
                            "sntp_time_sync", 
                            SNTP_TIME_SYNC_TASK_STACK_SIZE, 
                            NULL, 
                            SNTP_TIME_SYNC_TASK_PRIORITY,
                            NULL,
                            SNTP_TIME_SYNC_TASK_CORE_ID
                            );
}