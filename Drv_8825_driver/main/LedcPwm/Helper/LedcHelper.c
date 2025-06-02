#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "math.h"
#include <inttypes.h>

static char *TAG = "Helper class";

ledc_timer_bit_t m_timerResolution;

int m_maxFrequency = 5000000;

void SetUpHelper(ledc_timer_bit_t timerResolution)
{
    m_timerResolution = timerResolution;
}

/// @brief This function is for setting up the duty cycle based on the resolution by using the percentages insdate of the numbers
/// @param percentage  the percentage you want to set up the pwm signal
/// @return 
int CycleInPercentage(int percentage)
{
    if(percentage == 0)
    {
        return 0;
    }
    int resolutionfrombits = (int)pow(2, m_timerResolution);
    int targetResolution = (int)(resolutionfrombits * ((double)percentage / 100));
    ESP_LOGI(TAG, "The resolution is %d", targetResolution);
    return targetResolution;
}