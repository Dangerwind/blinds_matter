#include "button_handler.h"
#include "motor_control.h"
#include "matter_handler.h"
#include "config.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_matter.h"

static const char *TAG = "buttons";

typedef struct {
    gpio_num_t pin;
    bool       last_stable;
    bool       raw_last;
    uint32_t   raw_change_tick;
} button_t;

typedef enum {
    BTN_IDX_B1_OPEN  = 0,
    BTN_IDX_B1_CLOSE = 1,
    BTN_IDX_B2_OPEN  = 2,
    BTN_IDX_B2_CLOSE = 3,
    BTN_IDX_EMSTOP   = 4,
    BTN_COUNT        = 5
} btn_index_t;

static button_t s_buttons[BTN_COUNT] = {
    { .pin = BUTTON1_OPEN  },
    { .pin = BUTTON1_CLOSE },
    { .pin = BUTTON2_OPEN  },
    { .pin = BUTTON2_CLOSE },
    { .pin = EMERGENCY_STOP }
};

// ── Recommission state ────────────────────────────────────────────────────────
#define RECOMMISSION_HOLD_MS  15000   // удержание для перекомиссионирования
#define LED_BLINK_PERIOD_MS   250     // 4 вспышки в секунду (250мс on/off)

static bool     s_emstop_held       = false;  // кнопка сейчас зажата
static uint32_t s_emstop_press_tick = 0;      // когда нажали
static bool     s_recommissioning   = false;  // идёт режим перекомиссионирования

void button_handler_init(void)
{
    ESP_LOGI(TAG, "Initialising button GPIO");

    gpio_config_t cfg = {
        .pin_bit_mask = 0,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };

    // LED пины — OUTPUT для мигания
    cfg.pin_bit_mask = (1ULL << LED1_STATUS) | (1ULL << LED2_STATUS);
    gpio_config(&cfg);
    gpio_set_level(LED1_STATUS, 0);
    gpio_set_level(LED2_STATUS, 0);

    gpio_config_t in_cfg = {
        .pin_bit_mask = 0,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };

    for (int i = 0; i < BTN_COUNT; i++) {
        in_cfg.pin_bit_mask = 1ULL << s_buttons[i].pin;
        gpio_config(&in_cfg);
        bool current = (gpio_get_level(s_buttons[i].pin) == 1);
        s_buttons[i].last_stable     = current;
        s_buttons[i].raw_last        = current;
        s_buttons[i].raw_change_tick = 0;
    }

    ESP_LOGI(TAG, "Buttons ready");
}

static void _dispatch_press(btn_index_t idx)
{
    if (s_recommissioning) return;  // в режиме перекомиссионирования кнопки не работают

    switch (idx) {
        case BTN_IDX_B1_OPEN:
            ESP_LOGI(TAG, "Button: Blinds 1 OPEN");
            motor_start(MOTOR_CHANNEL_1, MOTOR_DIR_OPEN);
            break;
        case BTN_IDX_B1_CLOSE:
            ESP_LOGI(TAG, "Button: Blinds 1 CLOSE");
            motor_start(MOTOR_CHANNEL_1, MOTOR_DIR_CLOSE);
            break;
        case BTN_IDX_B2_OPEN:
            ESP_LOGI(TAG, "Button: Blinds 2 OPEN");
            motor_start(MOTOR_CHANNEL_2, MOTOR_DIR_OPEN);
            break;
        case BTN_IDX_B2_CLOSE:
            ESP_LOGI(TAG, "Button: Blinds 2 CLOSE");
            motor_start(MOTOR_CHANNEL_2, MOTOR_DIR_CLOSE);
            break;
        case BTN_IDX_EMSTOP:
            ESP_LOGW(TAG, "Button: EMERGENCY STOP");
            motor_stop_all();
            break;
        default:
            break;
    }
}

static void _start_recommissioning(void)
{
    ESP_LOGW(TAG, "Starting recommissioning...");
    s_recommissioning = true;
    motor_stop_all();

    // Сбросить fabric и открыть commissioning window
    esp_matter::factory_reset();
}

void button_handler_poll(void)
{
    uint32_t now = xTaskGetTickCount();

    // ── Мигание LED в режиме перекомиссионирования ────────────────────────
    if (s_recommissioning) {
        uint32_t ms = now * portTICK_PERIOD_MS;
        int led_state = (ms / LED_BLINK_PERIOD_MS) % 2;
        gpio_set_level(LED1_STATUS, led_state);
        gpio_set_level(LED2_STATUS, led_state);
    }

    // ── Обработка кнопок ──────────────────────────────────────────────────
    for (int i = 0; i < BTN_COUNT; i++) {
        button_t *b = &s_buttons[i];
        bool raw = (gpio_get_level(b->pin) == 1);

        if (raw != b->raw_last) {
            b->raw_last        = raw;
            b->raw_change_tick = now;
        }

        uint32_t elapsed_ms = (now - b->raw_change_tick) * portTICK_PERIOD_MS;
        if (elapsed_ms >= DEBOUNCE_MS && raw != b->last_stable) {
            b->last_stable = raw;

            if (!raw) {
                // Нажатие
                if (i == BTN_IDX_EMSTOP) {
                    s_emstop_held       = true;
                    s_emstop_press_tick = now;
                    if (!s_recommissioning) {
                        // Короткое нажатие — стоп
                        motor_stop_all();
                        ESP_LOGW(TAG, "Button: EMERGENCY STOP");
                    }
                } else {
                    _dispatch_press((btn_index_t)i);
                }
            } else {
                // Отпускание EMSTOP
                if (i == BTN_IDX_EMSTOP) {
                    s_emstop_held = false;
                }
            }
        }

        // ── Проверка удержания EMSTOP ─────────────────────────────────────
        if (i == BTN_IDX_EMSTOP && s_emstop_held && !s_recommissioning) {
            uint32_t held_ms = (now - s_emstop_press_tick) * portTICK_PERIOD_MS;
            if (held_ms >= RECOMMISSION_HOLD_MS) {
                _start_recommissioning();
            }
        }
    }
}
