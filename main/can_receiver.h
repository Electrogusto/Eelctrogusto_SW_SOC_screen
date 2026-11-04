#pragma once
#include "gauge_data.h"
#include "esp_err.h"
esp_err_t can_receiver_start(void);
void gauge_data_get(gauge_data_t *out);
void request_can(uint8_t MS, uint8_t ps, uint8_t CV, uint8_t TH, uint8_t SOC, uint8_t CM);
