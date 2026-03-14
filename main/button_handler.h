#pragma once
// ==============================================================================
// button_handler.h
// Public API for physical button handling (debounce, edge detection)
// ==============================================================================

/**
 * @brief  Initialise GPIO for all control buttons with debounce.
 *         Must be called once during setup, after motor_init().
 */
void button_handler_init(void);

/**
 * @brief  Poll button states and fire motor commands on short-press edges.
 *         Call from a periodic FreeRTOS task every ~10 ms.
 *
 * Handled actions:
 *   BUTTON1_OPEN  → motor_start(CHANNEL_1, OPEN)
 *   BUTTON1_CLOSE → motor_start(CHANNEL_1, CLOSE)
 *   BUTTON2_OPEN  → motor_start(CHANNEL_2, OPEN)
 *   BUTTON2_CLOSE → motor_start(CHANNEL_2, CLOSE)
 *   EMERGENCY_STOP → motor_stop_all()
 */
void button_handler_poll(void);
