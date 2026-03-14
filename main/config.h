#pragma once
// ==============================================================================
// config.h
// Central configuration: GPIO pins, timing constants, Matter parameters
// ==============================================================================

#include "driver/gpio.h"

// ── Motor driver (MX1508) pins ────────────────────────────────────────────────
// Each motor needs two pins: IN1 and IN2 on the MX1508 module.
// Forward:  IN1=HIGH, IN2=LOW
// Reverse:  IN1=LOW,  IN2=HIGH
// Stop:     IN1=LOW,  IN2=LOW
#define MOTOR1_IN1   GPIO_NUM_12
#define MOTOR1_IN2   GPIO_NUM_13
#define MOTOR2_IN1   GPIO_NUM_14
#define MOTOR2_IN2   GPIO_NUM_15

// ── Limit switches (reed switches / gercons) ─────────────────────────────────
// Active LOW: switch pulls pin to GND when magnet is present (blinds at limit).
// Internal pull-up resistors are enabled in software.
#define LIMIT1_OPEN  GPIO_NUM_4
#define LIMIT1_CLOSE GPIO_NUM_5
#define LIMIT2_OPEN  GPIO_NUM_16
#define LIMIT2_CLOSE GPIO_NUM_17

// ── Control buttons ───────────────────────────────────────────────────────────
// Active LOW with internal pull-ups.
#define BUTTON1_OPEN    GPIO_NUM_18
#define BUTTON1_CLOSE   GPIO_NUM_19
#define BUTTON2_OPEN    GPIO_NUM_23
#define BUTTON2_CLOSE   GPIO_NUM_22
#define EMERGENCY_STOP  GPIO_NUM_21

// ── Status LEDs ───────────────────────────────────────────────────────────────
// HIGH = LED on (motor running), LOW = LED off (motor stopped)
#define LED1_STATUS  GPIO_NUM_25
#define LED2_STATUS  GPIO_NUM_26

// ── Watchdog timer ────────────────────────────────────────────────────────────
// Maximum time (ms) a motor may run before auto-stop if no limit switch fires.
// Adjust this constant to match your blinds travel time + safety margin.
#define MOTOR_WATCHDOG_MS   50000   // 15 seconds

// ── Button debounce ───────────────────────────────────────────────────────────
#define DEBOUNCE_MS         50      // ms to ignore bouncing after first edge

// ── Matter device identity ────────────────────────────────────────────────────
// These values are used for commissioning. Change vendor/product IDs to your own
// registered values if submitting to the Matter Alliance certification.
#define MATTER_VENDOR_ID    0xFFF1  // Test vendor ID (development only)
#define MATTER_PRODUCT_ID   0x8001  // Custom product ID
#define MATTER_DEVICE_NAME  "ESP32 Blinds Controller"

// ── Number of blinds channels ────────────────────────────────────────────────
#define NUM_CHANNELS        2
