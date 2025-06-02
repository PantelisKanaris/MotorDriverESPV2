#ifndef HARDWARETIMERCONFIGURATION_H
#define HARDWARETIMERCONFIGURATION_H
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"


bool InitializeTimerSimple(int timerGroup, int timerId, int enableCounterReload, int direction, EventGroupHandle_t timerEventGroup, uint32_t *timerExpiredEvent);

void PauseTimer();

void SetUpTimerToStart();

void StartTimer(int timerInterval);

void StartTimerDouble(double timerInterval);


#endif