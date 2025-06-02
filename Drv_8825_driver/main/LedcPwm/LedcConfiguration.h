#ifndef LEDCONFIGURATION_H
#define LEDCONFIGURATION_H
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"

/// @brief This is the struct where the eventgroup will be stored where the fading event will notify when its done
typedef struct FadingEndEventNotifier
{
    EventGroupHandle_t *handleOfTheGroup; //the event group you want the bit trigger when the fading is completed 
    uint32_t *bitToTrigger; // the bit that will be triggered 
} FadingEndEventNotifier;

/// @brief Sets up the pwm without the fading function
/// @param speedMode 
/// @param dutyCycleResolution 
/// @param timer 
/// @param outputFrequency 
/// @param sourceClock 
/// @param channel 
/// @param fadeIntruptMode 
/// @param gpioNumber 
/// @param dutyCycle 
/// @param hpointValue 
void SetUpPwm(
    ledc_mode_t speedMode,  
    ledc_timer_bit_t dutyCycleResolution,
    ledc_timer_t timer,
    uint32_t outputFrequency,
    ledc_clk_cfg_t sourceClock,
    ledc_channel_t channel,
    ledc_intr_type_t fadeIntruptMode,
    int gpioNumber,
    uint32_t dutyCycle,
    int hpointValue
    );

/// @brief Sets up the pwm with the fading function
/// @param speedMode 
/// @param dutyCycleResolution 
/// @param timer 
/// @param outputFrequency 
/// @param sourceClock 
/// @param channel 
/// @param fadeIntruptMode 
/// @param gpioNumber 
/// @param dutyCycle 
/// @param hpointValue 
/// @param groupEvent 
void SetUpPwmWithFading(
    ledc_mode_t speedMode,
    ledc_timer_bit_t dutyCycleResolution,
    ledc_timer_t timer,
    uint32_t outputFrequency,
    ledc_clk_cfg_t sourceClock,
    ledc_channel_t channel,
    ledc_intr_type_t fadeIntruptMode,
    int gpioNumber,
    uint32_t dutyCycle,
    int hpointValue,
    FadingEndEventNotifier *groupEvent
);

/// @brief It registers the call back
void EnableFading();

/// @brief Sets the fading with the time variable
/// @param targetDytuCycle the target duty cycle you want to achieve 
/// @param timeInMs the time you want it to take to go to that dutyCycle
void SetFadingWithTime(int targetDytuCycle , int timeInMs);

/// @brief Sets the fading with the steps variable
/// @param targetdutyCycle the target duty cycle you want to achieve 
/// @param incriments at what incriments you want the duty cycle to incriment by 
/// @param numberOfCycles the number of cycles you want it to pass before it increments by the incriments parameter  
void SetFadingWithSteps(int targetdutyCycle,int incriments,int numberOfCycles);

void SetDutyCycle(uint32_t dutyCycle);

void SetFrequency(uint32_t frequency);

void RunForTimeSeconds(int time);

void MoveInSteps(double steps);



#endif