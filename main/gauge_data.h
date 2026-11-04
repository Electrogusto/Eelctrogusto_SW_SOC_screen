#pragma once
#include <stdbool.h>
#include <stdint.h>


typedef struct {
    int soc_percent;
    float power_kw;       // positive = traction, negative = regenerative braking
    float charge_kw;      
    int charge_state;
    int plug_state;     // there are 5 states (disconnected, connected, locked, active, waiting for disconect) The use of them depends on the setup
    bool can_data_valid;
    int64_t last_update_us;
    bool is_charging;
} gauge_data_t;
