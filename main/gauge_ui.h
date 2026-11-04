#pragma once
#include "gauge_data.h"
void gauge_ui_init(void);
void gauge_ui_update(const gauge_data_t *data);
void gauge_ui_update_driving(const gauge_data_t *data);
void gauge_ui_update_charging(const gauge_data_t *data);
void gauge_loading_screen(void);
void gauge_ui_show(void);