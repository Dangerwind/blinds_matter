#include "motor_control.h"
#include "button_handler.h"
#include "matter_handler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "main";

static void on_motor_stopped(motor_channel_t channel)
{
    // Сообщить Matter что мотор остановился → переключатели в HA → OFF
    matter_report_motor_stopped(channel == MOTOR_CHANNEL_1 ? 1 : 2);
}

static void poll_task(void *arg)
{
    ESP_LOGI(TAG, "Poll task started");
    while (true) {
        button_handler_poll();
        motor_watchdog_tick();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Blinds Controller — booting");

    motor_init();
    motor_set_stopped_callback(on_motor_stopped);
    button_handler_init();

    xTaskCreate(poll_task, "poll_task", 4096, nullptr, 5, nullptr);

    matter_handler_init();
    matter_handler_start();

    ESP_LOGI(TAG, "app_main complete");
}
