// ==============================================================================
// motor_control.cpp
// Motor driver logic for MX1508 dual H-bridge module.
//
// MX1508 truth table per channel:
//   IN1=0, IN2=0 → Stop (coast)
//   IN1=1, IN2=0 → Forward (OPEN direction)
//   IN1=0, IN2=1 → Reverse (CLOSE direction)
//   IN1=1, IN2=1 → Brake (avoid — can damage driver)
// ==============================================================================

#include "motor_control.h"
#include "config.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "motor";

// ── Internal per-channel hardware mapping ─────────────────────────────────────
typedef struct {
    gpio_num_t in1;           // Motor driver IN1 pin
    gpio_num_t in2;           // Motor driver IN2 pin
    gpio_num_t limit_open;    // Reed switch — open limit
    gpio_num_t limit_close;   // Reed switch — close limit
    gpio_num_t led;           // Status LED
} motor_hw_t;

// Hardware pin assignment for each channel
static const motor_hw_t s_hw[NUM_CHANNELS] = {
    {   // Channel 0 — Blinds 1
        .in1         = MOTOR1_IN1,
        .in2         = MOTOR1_IN2,
        .limit_open  = LIMIT1_OPEN,
        .limit_close = LIMIT1_CLOSE,
        .led         = LED1_STATUS
    },
    {   // Channel 1 — Blinds 2
        .in1         = MOTOR2_IN1,
        .in2         = MOTOR2_IN2,
        .limit_open  = LIMIT2_OPEN,
        .limit_close = LIMIT2_CLOSE,
        .led         = LED2_STATUS
    }
};

// Runtime state for each channel
static motor_state_t s_state[NUM_CHANNELS] = {};
static motor_stopped_cb_t s_stopped_cb = nullptr;

void motor_set_stopped_callback(motor_stopped_cb_t cb) { s_stopped_cb = cb; }

// ── Internal helpers ──────────────────────────────────────────────────────────

/**
 * Read a limit switch state.
 * Switches are active LOW (pull-up enabled): returns true when triggered (GND).
 */
static inline bool _limit_active(gpio_num_t pin)
{
    return (gpio_get_level(pin) == 0);
}

/**
 * Apply IN1/IN2 levels directly to the MX1508.
 */
static void _set_motor_pins(motor_channel_t ch, int in1, int in2)
{
    gpio_set_level(s_hw[ch].in1, in1);
    gpio_set_level(s_hw[ch].in2, in2);
}

/**
 * Cut power to motor and turn off status LED.
 * Updates internal state — does NOT log (callers log where appropriate).
 */
static void _stop_motor_hw(motor_channel_t ch)
{
    _set_motor_pins(ch, 0, 0);
    gpio_set_level(s_hw[ch].led, 0);
    s_state[ch].is_running = false;
    s_state[ch].direction  = MOTOR_DIR_STOP;
    if (s_stopped_cb) s_stopped_cb((motor_channel_t)ch);
}

// ── GPIO configuration ────────────────────────────────────────────────────────

void motor_init(void)
{
    ESP_LOGI(TAG, "Initialising motor GPIO");

    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        // ── Motor output pins ──────────────────────────────────────────────
        gpio_config_t out_cfg = {
            .pin_bit_mask = (1ULL << s_hw[ch].in1) | (1ULL << s_hw[ch].in2),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE
        };
        gpio_config(&out_cfg);

        // Start in stopped state
        _set_motor_pins((motor_channel_t)ch, 0, 0);

        // ── Status LED output ──────────────────────────────────────────────
        gpio_config_t led_cfg = {
            .pin_bit_mask = (1ULL << s_hw[ch].led),
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE
        };
        gpio_config(&led_cfg);
        gpio_set_level(s_hw[ch].led, 0);

        // ── Limit switch inputs (active LOW, internal pull-up) ─────────────
        gpio_config_t lim_cfg = {
            .pin_bit_mask = (1ULL << s_hw[ch].limit_open) |
                            (1ULL << s_hw[ch].limit_close),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type    = GPIO_INTR_DISABLE
        };
        gpio_config(&lim_cfg);

        // Initialise state struct
        s_state[ch] = (motor_state_t){};
    }

    ESP_LOGI(TAG, "Motor GPIO ready");
}

// ── Motor start ───────────────────────────────────────────────────────────────

void motor_start(motor_channel_t channel, motor_dir_t dir)
{
    if (channel >= NUM_CHANNELS) return;

    // Refresh limit switch state before acting
    bool at_open  = _limit_active(s_hw[channel].limit_open);
    bool at_close = _limit_active(s_hw[channel].limit_close);
    s_state[channel].at_open  = at_open;
    s_state[channel].at_close = at_close;

    // Refuse to move if already at the requested limit
    if (dir == MOTOR_DIR_OPEN && at_open) {
        ESP_LOGW(TAG, "Ch%d: already at OPEN limit — ignoring", channel + 1);
        return;
    }
    if (dir == MOTOR_DIR_CLOSE && at_close) {
        ESP_LOGW(TAG, "Ch%d: already at CLOSE limit — ignoring", channel + 1);
        return;
    }

    ESP_LOGI(TAG, "Ch%d: starting motor → %s",
             channel + 1, dir == MOTOR_DIR_OPEN ? "OPEN" : "CLOSE");

    // Drive MX1508 pins
    if (dir == MOTOR_DIR_OPEN) {
        _set_motor_pins(channel, 1, 0);   // IN1=H, IN2=L → forward
    } else {
        _set_motor_pins(channel, 0, 1);   // IN1=L, IN2=H → reverse
    }

    // Update state and start watchdog timer
    s_state[channel].is_running  = true;
    s_state[channel].direction   = dir;
    s_state[channel].start_tick  = xTaskGetTickCount();

    // Light up the status LED
    gpio_set_level(s_hw[channel].led, 1);
}

// ── Motor stop ────────────────────────────────────────────────────────────────

void motor_stop(motor_channel_t channel)
{
    if (channel >= NUM_CHANNELS) return;
    if (!s_state[channel].is_running) return;

    ESP_LOGI(TAG, "Ch%d: stopping motor", channel + 1);
    _stop_motor_hw(channel);
}

void motor_stop_all(void)
{
    ESP_LOGW(TAG, "EMERGENCY STOP — stopping all motors");
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        _stop_motor_hw((motor_channel_t)ch);
    }
}

// ── State query ───────────────────────────────────────────────────────────────

const motor_state_t *motor_get_state(motor_channel_t channel)
{
    return &s_state[channel];
}

// ── Watchdog tick (call periodically from task) ───────────────────────────────

void motor_watchdog_tick(void)
{
    uint32_t now = xTaskGetTickCount();

    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        if (!s_state[ch].is_running) continue;

        // ── Check limit switches ──────────────────────────────────────────
        bool at_open  = _limit_active(s_hw[ch].limit_open);
        bool at_close = _limit_active(s_hw[ch].limit_close);
        s_state[ch].at_open  = at_open;
        s_state[ch].at_close = at_close;

        bool limit_hit = (s_state[ch].direction == MOTOR_DIR_OPEN  && at_open) ||
                         (s_state[ch].direction == MOTOR_DIR_CLOSE && at_close);

        if (limit_hit) {
            ESP_LOGI(TAG, "Ch%d: limit switch triggered — stopping", ch + 1);
            _stop_motor_hw((motor_channel_t)ch);
            continue;
        }

        // ── Check watchdog timeout ────────────────────────────────────────
        uint32_t elapsed_ms = (now - s_state[ch].start_tick) * portTICK_PERIOD_MS;
        if (elapsed_ms >= MOTOR_WATCHDOG_MS) {
            ESP_LOGW(TAG, "Ch%d: WATCHDOG timeout after %lu ms — force stop", ch + 1, (unsigned long)elapsed_ms);
            _stop_motor_hw((motor_channel_t)ch);
        }
    }
}
