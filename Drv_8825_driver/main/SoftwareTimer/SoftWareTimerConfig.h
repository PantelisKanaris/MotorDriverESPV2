#ifndef SOFTWARETIMERCONFIG_H
#define SOFTWARETIMERCONFIG_H
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
void CreateTimer(void * CallBackFunction,int time);
void StartTimer();
#endif