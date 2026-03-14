#pragma once
#include "esp_err.h"

void matter_handler_init(void);
void matter_handler_start(void);

// Вызывать после остановки мотора чтобы синхронизировать состояние в HA
void matter_report_motor_stopped(int channel);
