#ifndef PULSECOUNTERCONFIG_H
#define PULSECOUNTERCONFIG_H
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"

#endif

void CreatePwmCounter(
    int unit, 
    int channel, 
    int inputPin, 
    int controllingPin, 
    int steps, int mode, 
    EventGroupHandle_t counterEventGroup,
    uint32_t *counterFinishedEvent,
    ledc_timer_config_t * timer,
    ledc_channel_config_t * chanel
    );
void DisableCounter();
void StartCounter(int steps);
void DisplayPcntUnitInfo();
void IRAM_ATTR PwmCounterIsrHandler(void *arg);