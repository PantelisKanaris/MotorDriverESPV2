#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "driver/pcnt.h"
#include "driver/ledc.h"
#include "PulseCounterConfig.h"
#include <inttypes.h>

static char *TAG = "Pulse counter config:";

pcnt_config_t *m_pwmCounterConfig;
EventGroupHandle_t m_CounterPwmEventGroup;
uint32_t *m_CounterCompletedBit;
ledc_timer_config_t *m_timerConfig;
ledc_channel_config_t *m_ChannelConfig;

void CreatePwmCounter(
    int unit,
    int channel,
    int inputPin,
    int controllingPin,
    int steps, 
    int mode,
    EventGroupHandle_t counterEventGroup,
    uint32_t *counterFinishedEvent,
    ledc_timer_config_t *timer,
    ledc_channel_config_t *channelConfig)
{
    m_timerConfig = timer;
    m_ChannelConfig = channelConfig;
    ESP_LOGI(TAG, "Starting the setup\n");

    m_pwmCounterConfig = (pcnt_config_t *)malloc(sizeof(pcnt_config_t));
    if (m_pwmCounterConfig == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for pcnt_config_t\n");
        return;
    }

    m_pwmCounterConfig->unit = unit;
    m_pwmCounterConfig->channel = channel == 0 ? PCNT_CHANNEL_0 : PCNT_CHANNEL_1;
    m_pwmCounterConfig->pulse_gpio_num = inputPin;
    m_pwmCounterConfig->ctrl_gpio_num = controllingPin;
    m_pwmCounterConfig->pos_mode = PCNT_COUNT_INC;      // Count up on the positive edge
    m_pwmCounterConfig->neg_mode = PCNT_COUNT_DIS;      // Keep the counter value on the negative edge
    m_pwmCounterConfig->lctrl_mode = PCNT_MODE_KEEP;    // Keep the counting direction if low
    m_pwmCounterConfig->hctrl_mode = PCNT_MODE_KEEP;    // Keep the primary counter mode if high
    m_pwmCounterConfig->counter_h_lim = 5000;
    m_pwmCounterConfig->counter_l_lim = -5000;

    pcnt_unit_config(m_pwmCounterConfig);
    pcnt_set_filter_value(m_pwmCounterConfig->unit, 96);
    pcnt_filter_enable(m_pwmCounterConfig->unit);

    DisableCounter();

    m_CounterPwmEventGroup = counterEventGroup;
    m_CounterCompletedBit = counterFinishedEvent;

    esp_err_t err = pcnt_isr_service_install(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to install PCNT ISR service, error: %s", esp_err_to_name(err));
        return;
    }

    err = pcnt_isr_handler_add(unit, PwmCounterIsrHandler, (void *)unit);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "PCNT ISR registered successfully.");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to register PCNT ISR, error: %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Finished the setup\n");
}

void DisableCounter()
{
    pcnt_counter_pause(m_pwmCounterConfig->unit);
    pcnt_counter_clear(m_pwmCounterConfig->unit);
}

void StartCounter(int steps)
{
    ESP_LOGI(TAG, "Starting counter with %d steps", steps);

    pcnt_set_event_value(m_pwmCounterConfig->unit, PCNT_EVT_THRES_1, steps);
    pcnt_event_enable(m_pwmCounterConfig->unit, PCNT_EVT_THRES_1);
    
    DisableCounter();
    pcnt_counter_resume(m_pwmCounterConfig->unit);

    uint32_t status = 0;
    pcnt_get_event_status(m_pwmCounterConfig->unit, &status);
    ESP_LOGI(TAG, "EVENT STATUS: %"PRIu32, status);
}

void IRAM_ATTR PwmCounterIsrHandler(void *arg)
{
    ledc_stop(m_timerConfig->speed_mode, m_ChannelConfig->channel, 0);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(m_CounterPwmEventGroup, *m_CounterCompletedBit, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken != pdFALSE)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void DisplayPcntUnitInfo()
{
    int16_t count = 0;
    esp_err_t err = pcnt_get_counter_value(m_pwmCounterConfig->unit, &count);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read PCNT counter value, error: %d", err);
        return;
    }

    ESP_LOGI(TAG, "Current PCNT count for unit %d: %"PRId16, m_pwmCounterConfig->unit, count);

    ESP_LOGI(TAG, "PCNT unit %d configuration:", m_pwmCounterConfig->unit);
    ESP_LOGI(TAG, "  Pulse GPIO number: %d", m_pwmCounterConfig->pulse_gpio_num);
    ESP_LOGI(TAG, "  Control GPIO number: %d", m_pwmCounterConfig->ctrl_gpio_num);
    ESP_LOGI(TAG, "  Positive edge counting mode: %d", m_pwmCounterConfig->pos_mode);
    ESP_LOGI(TAG, "  Negative edge counting mode: %d", m_pwmCounterConfig->neg_mode);
    ESP_LOGI(TAG, "  Control mode when low: %d", m_pwmCounterConfig->lctrl_mode);
    ESP_LOGI(TAG, "  Control mode when high: %d", m_pwmCounterConfig->hctrl_mode);
    ESP_LOGI(TAG, "  Counter high limit: %"PRId16, m_pwmCounterConfig->counter_h_lim);
    ESP_LOGI(TAG, "  Counter low limit: %"PRId16, m_pwmCounterConfig->counter_l_lim);
}