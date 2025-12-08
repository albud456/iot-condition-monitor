Overview

This project configures an ESP32 as both a SoftAP (Access Point) and Station, running an HTTP server. A FreeRTOS task with a queue handles Wi-Fi events and server start commands, while an RGB LED indicates connection and server status.