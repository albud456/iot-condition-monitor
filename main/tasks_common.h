/**
 * @file tasks_common.h
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef TASKS_COMMON_H_
#define TASKS_COMMON_H_

//wifi app task
#define WIFI_APP_TASK_STACK_SIZE        4096
#define WIFI_APP_TASK_PRIORITY          5
#define WIFI_APP_TASK_CORE_ID           0 //wifi should run on core 0

//http server task
#define HTTP_SERVER_TASK_STACK_SIZE     8192
#define HTTP_SERVER_TASK_PRIORITY       4
#define HTTP_SERVER_TASK_CORE_ID        0

//http monitor task
#define HTTP_SERVER_MONITOR_STACK_SIZE  4096
#define HTTP_SERVER_MONITOR_PRIORITY    3
#define HTTP_SERVER_MONITOR_CORE_ID     0

#endif