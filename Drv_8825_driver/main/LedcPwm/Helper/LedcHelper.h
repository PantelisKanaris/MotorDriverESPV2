#ifndef LEDCHELPER_H
#define LEDCHELPER_H
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
int CycleInPercentage(int percentage);
void SetUpHelper(ledc_timer_bit_t timerResolution);
#endif