#pragma once
// ==============================================================================
// config.h
// Central configuration: GPIO pins, timing constants, Matter parameters
// ==============================================================================

#include "driver/gpio.h"

// ── Motor driver (DRV8833) pins ───────────────────────────────────────────────
// Each motor needs two pins: AIN1/AIN2 for motor 1, BIN1/BIN2 for motor 2
// Forward:  xIN1=HIGH, xIN2=LOW
// Reverse:  xIN1=LOW,  xIN2=HIGH
// Stop:     xIN1=LOW,  xIN2=LOW (coast)
#define MOTOR1_IN1   GPIO_NUM_18   // AIN1
#define MOTOR1_IN2   GPIO_NUM_19   // AIN2
#define MOTOR2_IN1   GPIO_NUM_23   // BIN1
#define MOTOR2_IN2   GPIO_NUM_13   // BIN2

// ── Limit switches (reed switches / gercons) ──────────────────────────────────
// Active LOW: switch pulls pin to GND when magnet is present (blinds at limit).
// Internal pull-up resistors are enabled in software.
#define LIMIT1_OPEN  GPIO_NUM_16   // Геркон 1 — шторы сверху
#define LIMIT1_CLOSE GPIO_NUM_4    // Геркон 1 — шторы внизу
#define LIMIT2_OPEN  GPIO_NUM_27   // Геркон 2 — шторы сверху
#define LIMIT2_CLOSE GPIO_NUM_15   // Геркон 2 — шторы внизу

// ── Control buttons ───────────────────────────────────────────────────────────
// Active LOW with internal pull-ups.
#define BUTTON1_OPEN    GPIO_NUM_26   // Кнопка 1 вверх
#define BUTTON1_CLOSE   GPIO_NUM_17   // Кнопка 1 вниз
#define BUTTON2_OPEN    GPIO_NUM_33   // Кнопка 2 вверх
#define BUTTON2_CLOSE   GPIO_NUM_22   // Кнопка 2 вниз
#define EMERGENCY_STOP  GPIO_NUM_21   // Кнопка Стоп

// ── Status LEDs ───────────────────────────────────────────────────────────────
// HIGH = LED on, LOW = LED off
#define LED1_STATUS  GPIO_NUM_25
#define LED2_STATUS  GPIO_NUM_32

// ── Watchdog timer ────────────────────────────────────────────────────────────
// Maximum time (ms) a motor may run before auto-stop if no limit switch fires.
#define MOTOR_WATCHDOG_MS   50000   // 15 seconds

// ── Button debounce ───────────────────────────────────────────────────────────
#define DEBOUNCE_MS         50      // ms to ignore bouncing after first edge

// ── Matter device identity ────────────────────────────────────────────────────
#define MATTER_VENDOR_ID    0xFFF1
#define MATTER_PRODUCT_ID   0x8001
#define MATTER_DEVICE_NAME  "Blinds Controller"

// ── Number of blinds channels ─────────────────────────────────────────────────
#define NUM_CHANNELS        2