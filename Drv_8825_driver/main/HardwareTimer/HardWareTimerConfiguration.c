#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include "driver/timer.h"

static char *TAG = "Hard ware timer";
#define TIMER_BASE_CLK 80000000  // 80 MHz APB clock
#define TIMER_DIVIDER (16) //this is the diver we have 80kz ticks and we divide by 16 
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds


timer_group_t m_TimerGroup; //this is the timer group id we set it up in the initialization and is global because we use it everywhere 

/// @brief This is the static struct that is used to configure the timer.In the Initialization function depending on the parameters we change the values.
static timer_config_t m_TimerConfig = {
    .divider = 16,                       // Example divider
    .counter_dir = TIMER_COUNT_UP,       // Default direction
    .auto_reload = TIMER_AUTORELOAD_DIS, // Default auto-reload
    .alarm_en = TIMER_ALARM_DIS,         // Default alarm ..this need to stay because if not it will start apon initilalazation.
    .counter_en = TIMER_PAUSE,           // Start with the timer paused ..this need to stay because if not it will start apon initilalazation.
};

timer_idx_t m_TimerId; //this is the timer  id we set it up in the initialization and is global because we use it everywhere 

EventGroupHandle_t TestEventGroup; // this is the event group handle that is used to alert the calling class that the alarm has finished.

uint32_t *timerExpiredBit; // this is the bit that is triggered.

/// @brief This checks the timer status is used in the set up timer function
void CheckTimerStatus();
/// @brief Same to this it checks if the timer is running using te set up  timer  value and the interval (the time it need to go to set the alarm ) 
/// @param last_timer_value 
/// @param timerInterval 
void CheckIfTimerIsRunning(int *last_timer_value, int *timerInterval);


/// @brief 
/// @param timerGroup 
/// @param timerId 
/// @param enableCounterReload 
/// @param direction 
/// @param timerEventGroup 
/// @param timerExpiredEvent 
/// @return 
bool InitializeTimerSimple(int timerGroup, int timerId, int enableCounterReload, int direction, EventGroupHandle_t timerEventGroup, uint32_t *timerExpiredEvent)
{
    // Determine timer group
    m_TimerGroup = (timerGroup == 1) ? TIMER_GROUP_1 : TIMER_GROUP_0;
    if (timerGroup == 1)
    {
        ESP_LOGI(TAG, "Configuring Timer Group 1");
    }
    else
    {
        ESP_LOGI(TAG, "Configuring Timer Group 0");
    }

    // Determine timer ID
    m_TimerId = (timerId == 1) ? TIMER_1 : TIMER_0;
    if (timerId == 1)
    {
        ESP_LOGI(TAG, "Using Timer 1");
    }
    else
    {
        ESP_LOGI(TAG, "Using Timer 0");
    }

    // Set timer direction it must be up because the timer is set to 0 to start and need to go to x(timer Interval value).
    m_TimerConfig.counter_dir = (direction == 1) ? TIMER_COUNT_UP : TIMER_COUNT_DOWN;

    // Set auto-reload feature based on input
    m_TimerConfig.auto_reload = (enableCounterReload == 1) ? TIMER_AUTORELOAD_EN : TIMER_AUTORELOAD_DIS;

    // Log the configuration for debugging
    ESP_LOGI(TAG, "Timer direction: %s, Auto-reload: %s",
             (m_TimerConfig.counter_dir == TIMER_COUNT_UP) ? "Up" : "Down",
             (m_TimerConfig.auto_reload == TIMER_AUTORELOAD_EN) ? "Enabled" : "Disabled");

    // Initialize the timer with the static configuration
    esp_err_t initResult = timer_init(m_TimerGroup, m_TimerId, &m_TimerConfig);
    if (initResult != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize timer: %s", esp_err_to_name(initResult));
        return false;
    }

    // Set the event group and event bit if the initialization was successful
    TestEventGroup = timerEventGroup;    // Assuming 'TestEventGroup' is globally accessible or part of a structure
    timerExpiredBit = timerExpiredEvent; // Similar scope assumption as above

    ESP_LOGI(TAG, "Timer initialized successfully");
    return true;
}

/// @brief This is the callback function that will be called once the timer has has reach the value given.
/// @param args 
/// @return 
void IRAM_ATTR timerISRCallBackFunction(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Assuming timerExpiredBit is a uint32_t bit mask and is properly defined globally
    // Use extern if these are defined in another file
    // ESP_LOGI(TAG, "ISR TRIGGERED");
    // Set bits from ISR
    xEventGroupSetBitsFromISR(TestEventGroup, *timerExpiredBit, &xHigherPriorityTaskWoken);

    // Check if a higher priority task was woken by this ISR
    if (xHigherPriorityTaskWoken != pdFALSE)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // Context switch to the higher priority task that was woken
    }

    // Logging in ISR is generally not recommended due to potential delays and issues in the interrupt context
    // ESP_LOGI(TAG,"Set bits\n");  // It's better to avoid logging inside ISR unless for debugging
}

/// @brief This is for pausing the timer if its neccessary.
void PauseTimer()
{
    timer_pause(m_TimerGroup, m_TimerId);
}

/// @brief This is for setting up the timer the first time it needs to be fired, otherwise if you are going to start the timer again this should not be called.
void SetUpTimerToStart()
{
    // Reset timer counter value to 0
    esp_err_t err = timer_set_counter_value(m_TimerGroup, m_TimerId, 0);
    int last_timer_value = 0;
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set timer counter value: %s", esp_err_to_name(err));
        return;
    }

    err = timer_set_alarm(m_TimerGroup, m_TimerId, 1);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable timer interrupt generation: %s", esp_err_to_name(err));
        return;
    }

    err = timer_enable_intr(m_TimerGroup, m_TimerId);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable timer interrupt: %s", esp_err_to_name(err));
        return;
    }

    err = timer_isr_callback_add(m_TimerGroup, m_TimerId, (void *)timerISRCallBackFunction, NULL, 0);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add timer ISR callback: %s", esp_err_to_name(err));
        return;
    }
    CheckTimerStatus();
}


void CheckTimerStatus()
{
    timer_config_t config;
    esp_err_t errr = timer_get_config(m_TimerGroup, m_TimerId, &config);
    if (errr == ESP_OK)
    {
        ESP_LOGI(TAG, "Timer Configuration:");
        ESP_LOGI(TAG, "Divider: %d", config.divider);
        ESP_LOGI(TAG, "Counter Direction: %s", (config.counter_dir == TIMER_COUNT_UP) ? "Up" : "Down");
        ESP_LOGI(TAG, "Auto Reload: %s", (config.auto_reload) ? "Enabled" : "Disabled");
        ESP_LOGI(TAG, "Counter Enabled: %s", (config.counter_en == TIMER_PAUSE) ? "Paused" : "Running");
        ESP_LOGI(TAG, "Alarm Enabled: %s", (config.alarm_en) ? "Enabled" : "Disabled");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get timer configuration: %s", esp_err_to_name(errr));
    }
}

void CheckIfTimerIsRunning(int *last_timer_value, int *timerInterval)
{
    uint64_t current_timer_value;
    timer_get_counter_value(m_TimerGroup, m_TimerId, &current_timer_value);

    if (current_timer_value != *last_timer_value)
    {
        ESP_LOGI(TAG, "Timer is running. Current value: %llu", current_timer_value);
        ESP_LOGI(TAG, "Timer is running.  value to reach: %d", *timerInterval * TIMER_SCALE);
    }
    else
    {
        ESP_LOGI(TAG, "Timer is paused or stopped.");
    }
}

/// @brief This function is to start the timer.Thi should be called by it self after the SetupTimer has been called.
/// @param timerInterval  the interval in seconds that you want the timer to trigger
void StartTimer(int timerInterval)
{
     // Reset timer counter value to 0
    esp_err_t err = timer_set_counter_value(m_TimerGroup, m_TimerId, 0);
    int last_timer_value = 0;
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set timer counter value: %s", esp_err_to_name(err));
        return;
    }

    /* Configure the alarm value and the interrupt on alarm. */
    err = timer_set_alarm_value(m_TimerGroup, m_TimerId, timerInterval * TIMER_SCALE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set timer alarm value: %s", esp_err_to_name(err));
        return;
    }

    // Start the timer
     err = timer_start(m_TimerGroup, m_TimerId);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Started hardware timer");
}

void StartTimerDouble(double timerInterval)
{

     // Reset timer counter value to 0
    esp_err_t err = timer_set_counter_value(m_TimerGroup, m_TimerId, 0);
    int last_timer_value = 0;
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set timer counter value: %s", esp_err_to_name(err));
        return;
    }

    /* Configure the alarm value and the interrupt on alarm. */
    err = timer_set_alarm_value(m_TimerGroup, m_TimerId, timerInterval * TIMER_SCALE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set timer alarm value: %s", esp_err_to_name(err));
        return;
    }

    // Start the timer
     err = timer_start(m_TimerGroup, m_TimerId);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Started hardware timer");
}

//Example of how you need to call it  after you have called Initilazied timer.

// ESP_LOGI(TAG, "Start timer\n");
//     if(TimerSetUp == false)
//     {
//         SetUpTimerToStart();
//         TimerSetUp=true;
//     }
//     StartTimer(5);
//     xEventGroupWaitBits(m_TimerRunTime, m_timerCompleted,pdTRUE,pdFALSE,portMAX_DELAY);