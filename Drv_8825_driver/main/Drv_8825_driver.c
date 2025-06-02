#include <stdio.h>
#include "LedcPwm/LedcConfiguration.h"
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <inttypes.h>

#define PIN 2
EventGroupHandle_t EventGroupNotifier;
FadingEndEventNotifier *eventNotifier;

// Those bits will be set once the fading has been completed.
uint32_t anotherEvent = BIT0;

static char *TAG = "Main";
void app_main(void)
{
    // set the gpio pin 2 which is the direction to 1
    // gpio_pad_select_gpio(PIN);
    // gpio_set_direction(PIN,GPIO_MODE_OUTPUT);
    // gpio_set_level(PIN,1);
    // int isON=0;

    EventGroupNotifier = xEventGroupCreate();
    if (EventGroupNotifier == NULL)
    {
        ESP_LOGE(TAG, "Failed to create EventGroupNotifier");
        return;
    }

    eventNotifier = (FadingEndEventNotifier *)malloc(sizeof(FadingEndEventNotifier));
    if (eventNotifier == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate miemory for event notifier.");
        return;
    }

    eventNotifier->bitToTrigger = &anotherEvent;
    eventNotifier->handleOfTheGroup = &EventGroupNotifier;

    SetUpPwm(
        LEDC_LOW_SPEED_MODE,
        LEDC_TIMER_13_BIT,
        LEDC_TIMER_0,
        200,
        LEDC_AUTO_CLK,
        LEDC_CHANNEL_0,
        LEDC_INTR_FADE_END,
        13,
        0,
        0);
    // ESP_LOGI(TAG, "i am here\n");
    // SetFadingWithTime(0, 3000);

    // ESP_LOGI(TAG, "waiting\n");
    // RunForTimeSeconds(5);
    // ESP_LOGI(TAG, "TimerCompleted again!\n");
    // ESP_LOGI(TAG, "waiting\n");
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // RunForTimeSeconds(5);
    // ESP_LOGI(TAG, "SUCCESSS THE EVENT HAS BEEN SUCCESFULLY TRIGGERD YOU CAN GO TO BED\n");
    gpio_set_direction(10, GPIO_MODE_OUTPUT);
    gpio_set_level(10, 1);
    MoveInSteps(800);
    ESP_LOGI(TAG, "1\n");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    //MoveInSteps(900);
    ESP_LOGI(TAG, "2\n");
    xEventGroupWaitBits(EventGroupNotifier, anotherEvent, pdTRUE, pdTRUE, portMAX_DELAY);

    while (1)
    {
        vTaskDelay(1);
    }
}