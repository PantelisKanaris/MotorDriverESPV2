#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "LedcConfiguration.h"
#include "esp_heap_caps.h"
#include "HardWareTimerConfiguration.h"
#include "Helper/LedcHelper.h"
#include "PulseCounterConfiguration/PulseCounterConfig.h"
#include <inttypes.h>

// This is for the logging
static char *TAG = " Ledc PWM setup";

// This is for tha fading
static uint32_t m_fadeFlags;
// These parameters has the status of the fading
int m_isFadingEnabled = 0;

ledc_timer_config_t *m_ledTimerConfig;

ledc_channel_config_t *m_ledChannelConfig;

uint32_t m_selectedFrequency;

// this is for checking when the callback function is called that means that the fading completed
EventGroupHandle_t m_LedcFadingEventHandle;
// this event group will tell when the timer has expired
EventGroupHandle_t m_TimerRunTime;

EventGroupHandle_t m_pwmCounterHandle;

bool TimerSetUp = false;

// Those bits will be set to one when the callback function is called.
const uint32_t m_fadingInTimeCompleted = BIT0;
const uint32_t m_fadingInStepsCompleted = BIT1;
const uint32_t m_fadingCompleted = BIT2;
const uint32_t m_PwmCounterCompleted = BIT3;

// the bit for the timer
const uint32_t m_timerCompleted = BIT0;

void DisableLedcDriver();

void print_heap_info(const char *tag)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    ESP_LOGI(tag, "Heap info: total_free_bytes=%u, total_allocated_bytes=%u, largest_free_block=%u",
             info.total_free_bytes, info.total_allocated_bytes, info.largest_free_block);
}

// Function to create and initialize the LED timer configuration structure
void CreateTheLedTimerConfigurationStruct(
    ledc_mode_t speedMode,
    ledc_timer_bit_t dutyCycleResolution,
    ledc_timer_t timer,
    uint32_t outputFrequency,
    ledc_clk_cfg_t sourceClock)
{
    m_selectedFrequency = outputFrequency;
    ESP_LOGI(TAG, "Started setting up the timer configuration\n");
    // Allocate memory for the timer configuration structure
    m_ledTimerConfig = (ledc_timer_config_t *)malloc(sizeof(ledc_timer_config_t));
    if (m_ledTimerConfig == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for timer configuration.");
    }
    // Assign values to the structure members
    m_ledTimerConfig->speed_mode = speedMode;
    m_ledTimerConfig->duty_resolution = dutyCycleResolution;
    m_ledTimerConfig->timer_num = timer;
    m_ledTimerConfig->freq_hz = outputFrequency;
    m_ledTimerConfig->clk_cfg = sourceClock;
    ESP_ERROR_CHECK(ledc_timer_config(m_ledTimerConfig));
    ESP_LOGI(TAG, "Finished setting up the timer configuration\n");
}

void ConfigureChannel(
    ledc_channel_t channel,
    ledc_intr_type_t fadeIntruptMode,
    int gpioNumber,
    uint32_t dutyCycle,
    int hpointValue)
{
    ESP_LOGI(TAG, "Setting up the channel configuration\n");
    // Allocate memory for the timer configuration structure
    m_ledChannelConfig = (ledc_channel_config_t *)malloc(sizeof(ledc_channel_config_t));
    if (m_ledChannelConfig == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for channel configuration.");
    }

    // Assign values to the structure members
    m_ledChannelConfig->speed_mode = m_ledTimerConfig->speed_mode;
    m_ledChannelConfig->channel = channel;
    m_ledChannelConfig->intr_type = fadeIntruptMode;
    m_ledChannelConfig->gpio_num = gpioNumber;
    m_ledChannelConfig->duty = dutyCycle;
    m_ledChannelConfig->hpoint = hpointValue;
    m_ledChannelConfig->timer_sel = m_ledTimerConfig->timer_num;
    ESP_ERROR_CHECK(ledc_channel_config(m_ledChannelConfig));
    ESP_LOGI(TAG, "Finished setting up the channel configuration\n");
}

/// @brief This tasks monitors/wait until the fade is completed once completed a bit will be set and the monitor will set a bit of the FadingEndEventNotifier
///  where that in turn will notify the parent class
/// @param args
void FadingMonitoringTask(void *args)
{
    FadingEndEventNotifier *groupEvent = (FadingEndEventNotifier *)args;
    ESP_LOGI(TAG, "Fading monitoring task started\n");

    if (m_LedcFadingEventHandle == NULL)
    {
        ESP_LOGE(TAG, "LedcFadingEventHandleEventGroup is NULL in FadingMonitoringTask");
        vTaskDelete(NULL); // Terminate the task
        return;
    }

    while (true) // Run indefinitely
    {
        // Wait for the fading completed event
        xEventGroupWaitBits(m_LedcFadingEventHandle, m_fadingCompleted, pdTRUE, pdFALSE, portMAX_DELAY);
        ESP_LOGI(TAG, "Fading has been completed setting the given bits in the given event group\n");

        // Check if event group and bit are not NULL before setting the bits
        if (groupEvent->handleOfTheGroup == NULL)
        {
            ESP_LOGE(TAG, "event group is NULL");
        }
        else if (groupEvent->bitToTrigger == NULL)
        {
            ESP_LOGE(TAG, "bit to trigger is NULL");
        }
        else
        {
            xEventGroupSetBits(*(groupEvent->handleOfTheGroup), *(groupEvent->bitToTrigger));
        }
    }
}

void TimerFunction(void *args)
{
    ESP_LOGI(TAG, "timer function triggered \n");
    SetDutyCycle(0);
    xEventGroupSetBits(m_TimerRunTime, m_timerCompleted);
}

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
    int hpointValue)
{
    m_TimerRunTime = xEventGroupCreate();
    m_pwmCounterHandle = xEventGroupCreate();
    InitializeTimerSimple(0, 0, 1, 1, m_TimerRunTime, &m_timerCompleted);
    CreateTheLedTimerConfigurationStruct(speedMode, dutyCycleResolution, timer, outputFrequency, sourceClock);
    ConfigureChannel(channel, fadeIntruptMode, gpioNumber, dutyCycle, hpointValue);
    CreatePwmCounter(0, 0, gpioNumber, 5, 400, 0, m_pwmCounterHandle, &m_PwmCounterCompleted, m_ledTimerConfig, m_ledChannelConfig);
    SetUpHelper(m_ledTimerConfig->duty_resolution);
    gpio_set_direction(gpioNumber, GPIO_MODE_INPUT_OUTPUT);
   // gpio_matrix_out(gpioNumber, LEDC_LS_SIG_OUT0_IDX + m_ledChannelConfig->channel, 0, 0); //TODO: Check to see if its needed.
}

// this function runs and will monitor when the fade has ended will trigger the bits for the monitor function which in turn will trigger the bits of another eventgroup
IRAM_ATTR bool CallBackFunction(const ledc_cb_param_t *param, void *user_arg)
{
    // Your callback logic here
    if (param->event == LEDC_FADE_END_EVT)
    {
        xEventGroupSetBits(m_LedcFadingEventHandle, m_fadingCompleted);
    }

    return false;
}

void EnableFading()
{
    if (m_ledChannelConfig == NULL)
    {
        ESP_LOGE(TAG, "ledc_channel is NULL");
        return;
    }
    if (m_ledChannelConfig->intr_type == LEDC_INTR_DISABLE)
    {
        ESP_LOGI(TAG, "Fading is disabled, please change it in the channel configuration\n");
        m_isFadingEnabled = 0;
        return;
    }

    if (m_LedcFadingEventHandle == NULL)
    {
        ESP_LOGE(TAG, "Failed to create fading event group");
        return;
    }
    ledc_cbs_t callbacks =
        {
            .fade_cb = CallBackFunction // user_arg
        };
    ledc_cb_register(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, &callbacks, (void *)0);
    ESP_LOGI(TAG, "Set up the fade\n");
    m_isFadingEnabled = 1;
    ESP_LOGI(TAG, "Fading has been enabled\n");
}

void SetFadingWithTime(int targetDytuCycle, int timeInMs)
{
    ESP_ERROR_CHECK(ledc_set_fade_with_time(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, targetDytuCycle, timeInMs));
    ESP_LOGI(TAG, "Set up the fade in ms\n");
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

void SetFadingWithSteps(int targetdutyCycle, int incriments, int numberOfCycles)
{
    ESP_ERROR_CHECK(ledc_set_fade_with_step(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, targetdutyCycle, incriments, numberOfCycles)); // every 1000 incrise the duty cycle by 100
    ESP_LOGI(TAG, "Set up the fade in STEPS\n");
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

void IntializeFading(FadingEndEventNotifier *groupEvent)
{
    m_LedcFadingEventHandle = xEventGroupCreate();
    xTaskCreate(FadingMonitoringTask, "MonitoringEventTask", 4096, (void *)groupEvent, 1, NULL);
    ESP_ERROR_CHECK(ledc_fade_func_install(m_fadeFlags));
}

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
    FadingEndEventNotifier *groupEvent)
{
    m_TimerRunTime = xEventGroupCreate();
    IntializeFading(groupEvent);
    CreateTheLedTimerConfigurationStruct(speedMode, dutyCycleResolution, timer, outputFrequency, sourceClock);
    ConfigureChannel(channel, fadeIntruptMode, gpioNumber, dutyCycle, hpointValue);
    EnableFading();
    SetUpHelper(m_ledTimerConfig->duty_resolution);
}

void SetDutyCycle(uint32_t dutyCycle)
{
    ledc_set_duty(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, dutyCycle);

    ledc_update_duty(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel);
}

void SetDutyCyclePercentage(int percentage)
{
    uint32_t dutyCycle = (uint32_t)CycleInPercentage(percentage);
    ledc_set_duty(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, dutyCycle);

    ledc_update_duty(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel);
}

void SetFrequency(uint32_t frequency)
{
    ledc_set_freq(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, frequency);
}

void RunForTimeSeconds(int time)
{

    // SetDutyCycle(4096);
    SetDutyCyclePercentage(50);
    SetFrequency(m_selectedFrequency);
    ESP_LOGI(TAG, "Start timer\n");
    if (TimerSetUp == false)
    {
        SetUpTimerToStart();
        TimerSetUp = true;
    }
    StartTimer(5);
    xEventGroupWaitBits(m_TimerRunTime, m_timerCompleted, pdTRUE, pdFALSE, portMAX_DELAY);
    DisableLedcDriver();
    ESP_LOGI(TAG, "End timer\n");
}

// Disable the driver.
void DisableLedcDriver()
{
    ledc_stop(m_ledTimerConfig->speed_mode, m_ledChannelConfig->channel, 0);
}

void MoveInSteps(double steps)
{
    StartCounter(steps);
    SetDutyCyclePercentage(50);
    SetFrequency(m_selectedFrequency);
    ESP_LOGI(TAG, "Counter value: %" PRIu32, m_selectedFrequency);
    ESP_LOGI(TAG, "Start counter\n");
    ESP_LOGI(TAG, "waiting");
    xEventGroupWaitBits(m_pwmCounterHandle, m_PwmCounterCompleted, pdTRUE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "End timer\n");
}