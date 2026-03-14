#pragma once
#include "driver/gpio.h"
#include "stdint.h"
#include "stdbool.h"

typedef enum { MOTOR_CHANNEL_1 = 0, MOTOR_CHANNEL_2 = 1 } motor_channel_t;
typedef enum { MOTOR_DIR_STOP = 0, MOTOR_DIR_OPEN, MOTOR_DIR_CLOSE } motor_dir_t;

#define NUM_CHANNELS 2

typedef struct {
    bool         is_running;
    motor_dir_t  direction;
    uint32_t     start_tick;
    bool         at_open;
    bool         at_close;
} motor_state_t;

// Callback вызывается когда мотор останавливается (watchdog или концевик)
typedef void (*motor_stopped_cb_t)(motor_channel_t channel);

void motor_init(void);
void motor_set_stopped_callback(motor_stopped_cb_t cb);
void motor_start(motor_channel_t channel, motor_dir_t dir);
void motor_stop(motor_channel_t channel);
void motor_stop_all(void);
void motor_watchdog_tick(void);
const motor_state_t *motor_get_state(motor_channel_t channel);
