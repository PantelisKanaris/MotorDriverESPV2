#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include <inttypes.h>

/// @brief This is the FreeRTOS softoware Timer with all of its limitation see the documentation for more details.
TimerHandle_t m_Timer;

/// @brief This functions creates a softwate timer with the time being calculated based in the the ticks of the proccessor witch are in 
/// ms thus having the 1000 so 1000ms = 1s  and after it expires it calls a callback function.
/// @param CallBackFunction 
/// @param time 
void CreateTimer(void * CallBackFunction,int time)
{
   m_Timer = xTimerCreate
    (
        "First Timer", // NAME OF THE TIMER 
        time * 1000/portTICK_PERIOD_MS, //EXPIRES EVERY time SECONDS
        pdFALSE, //RESETS EVERY TIME IT EXPIRES  
        (void *)0,//TIMER ID
        CallBackFunction
    );

}

void StartTimer()
{
    xTimerStart(m_Timer,portMAX_DELAY);
}