#include "can_receiver.h"
#include "gauge_config.h"
#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

static const char *TAG = "can_rx";
static gauge_data_t latest = {
    .soc_percent = -1, .power_kw = 0.0f, .can_data_valid = false, .last_update_us = 0,
    .charge_kw = 0, .charge_state = 0, .plug_state = 0, .is_charging = false
};
static SemaphoreHandle_t data_mutex;

// static void update_range(gauge_data_t *d)
// {
//     if (CONSUMPTION_KWH_PER_100KM > 0.01f) {
//         d->range_km = (d->soc_percent / 100.0f) *
//                       BATTERY_USABLE_KWH * 100.0f /
//                       CONSUMPTION_KWH_PER_100KM;
//     } else {
//         d->range_km = 0.0f;
//     }
// }

void gauge_data_get(gauge_data_t *out)
{
    if (!out || !data_mutex) return;
    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        *out = latest;
        xSemaphoreGive(data_mutex);
    }
}

// PLACEHOLDER DECODER:
// Replace the ID, byte order, scale factors and gear mapping below with
// the definitions from your vehicle's CAN database / sender firmware.
static bool decode_can_frame(const twai_message_t *msg, gauge_data_t *d)
{
#if CAN_DECODER_ENABLED
    if (msg->identifier != 0x293) {
        return false;
    }
    if (msg->data_length_code < 8) {
        return false;
    }
    // sort for appropriate message
    if (msg->data[0] == 0x01) {
        // mcu summary
        d->charge_kw = (msg->data[3] << 8) | (msg->data[2]);
        d->charge_state = msg->data[4];
        d->plug_state = msg->data[5];
        if (d->plug_state == 1) {
            d->is_charging = false;
        } else {
            d->is_charging = true;
        }
        printf("mcu summary: %fkW, charge state: %d, plug state: %d, is charging %d\n", d->charge_kw, d->charge_state, d->plug_state, d->is_charging);

        return true;
    } 

    if (msg->data[0] == 0x02) {
        // pack summary
        
        float voltage = ((msg->data[3] << 8) | (msg->data[2])) * 0.1;
        float current = ((msg->data[5] << 8) | (msg->data[4])) * 0.1;
        d->power_kw = voltage * current / 1000;
        printf("pack summary: %fV, %fA, %lfkW\n", voltage, current, d->power_kw);
        return true;
    }

    if (msg->data[0] == 0x05) {
        d->soc_percent = msg->data[1];
        if (d->soc_percent < 0 || d->soc_percent > 100) return false;
        printf("soc summary: %d\n", d->soc_percent);
        return true;
    }



#else
    (void)msg; (void)d;
#endif
    return false;
}

static void can_task(void *arg) {
    (void)arg;
    twai_message_t msg;
    while (1) {
        if (twai_receive(&msg, pdMS_TO_TICKS(250)) == ESP_OK) {
            gauge_data_t candidate;
            if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                candidate = latest;
                xSemaphoreGive(data_mutex);
                if (decode_can_frame(&msg, &candidate)) {
                    candidate.can_data_valid = true;
                    candidate.last_update_us = esp_timer_get_time();
                    // update_range(&candidate);
                    if (xSemaphoreTake(data_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
                        latest = candidate;
                        xSemaphoreGive(data_mutex);
                    }
                }
            }
        }
    }
}

esp_err_t can_receiver_start(void)
{
    data_mutex = xSemaphoreCreateMutex();
    if (!data_mutex) return ESP_ERR_NO_MEM;
    // update_range(&latest);

    const twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(PIN_CAN_TX, PIN_CAN_RX, TWAI_MODE_NORMAL);
    const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWAI install failed: %s", esp_err_to_name(err));
        return err;
    }
    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "TWAI start failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "TWAI started; decoder is %s",
             CAN_DECODER_ENABLED ? "enabled (example mapping)" : "disabled placeholder");
    if (xTaskCreate(can_task, "can_rx", 4096, NULL, 8, NULL) != pdPASS)
        return ESP_ERR_NO_MEM;
    return ESP_OK;
}


void request_can(uint8_t MS, uint8_t ps, uint8_t CV, uint8_t TH, uint8_t SOC, uint8_t CM) {
    twai_message_t message = {0};
    message.identifier = 0x313;
    message.data_length_code = 8;

    message.data[0] = 0x00;
    message.data[1] = MS;
    message.data[2] = ps;
    message.data[3] = CV;
    message.data[4] = TH;
    message.data[5] = SOC;
    message.data[6] = CM;
    message.data[7] = 0x00;

    esp_err_t err = twai_transmit(
        &message,
        pdMS_TO_TICKS(100)
    );

    printf("can request\n");
    
}
