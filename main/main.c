#include "gauge_config.h"
#include "gauge_ui.h"
#include "can_receiver.h"
#include "loading_screen.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include <stdlib.h>
#include "lvgl.h"
#include <stdbool.h>

static const char *TAG = "ev_gauge";
static esp_lcd_panel_handle_t panel_handle;
static lv_display_t *display_handle;

// Called from the SPI driver's completion context after the LCD transfer.
static bool lcd_color_trans_done(esp_lcd_panel_io_handle_t panel_io,
                                 esp_lcd_panel_io_event_data_t *edata,
                                 void *user_ctx)
{
    (void)panel_io;
    (void)edata;
    (void)user_ctx;

    if (display_handle != NULL) {
        lv_display_flush_ready(display_handle);
    }

    return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    lv_draw_sw_rgb565_swap(px_map, w * h);

    (void)disp;
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, px_map);

}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(2);
}

static volatile bool s_setup_done = false;
static volatile bool expired_data = false;

static void lvgl_task(void *arg)
{
    (void)arg;
    while (1) {
        // Keep all LVGL calls on this task; CAN reception only updates shared data.
        if (s_setup_done) {
            gauge_data_t data;
            gauge_data_get(&data);
            if (expired_data) {
                data.charge_kw = -1;
                data.power_kw = 0;
                data.soc_percent = -1;
            }
            gauge_ui_update(&data);
        }

        uint32_t wait_ms = lv_timer_handler();
        if (wait_ms < 5) wait_ms = 5;
        if (wait_ms > 20) wait_ms = 20;
        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

static esp_err_t display_init(void)
{
    gpio_config_t bl_conf = {
        .pin_bit_mask = 1ULL << PIN_LCD_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&bl_conf));
    gpio_set_level(PIN_LCD_BACKLIGHT, 1);

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = GAUGE_WIDTH * 40 * sizeof(uint16_t)
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_LCD_DC,
        .cs_gpio_num = PIN_LCD_CS,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = lcd_color_trans_done,
        .user_ctx = NULL
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                             &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // ST7789 native panel is 240x320. Swap XY for 320x240 landscape.
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    lv_init();
    display_handle = lv_display_create(GAUGE_WIDTH, GAUGE_HEIGHT);
    if (!display_handle) return ESP_ERR_NO_MEM;
    lv_display_set_color_format(display_handle, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display_handle, lvgl_flush_cb);

    // Two partial buffers; SPI DMA transfers are completed asynchronously.
    size_t buf_bytes = GAUGE_WIDTH * 40 * sizeof(uint16_t);
    void *buf1 = heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buf2 = heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!buf1 || !buf2) return ESP_ERR_NO_MEM;
    lv_display_set_buffers(display_handle, buf1, buf2, buf_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Attach the LVGL display handle to the transfer-completion callback.
    // The panel IO callback's user_ctx is set at creation, so this callback
    // uses a global display handle if callback arrives after initialisation.
    // (See lcd_color_trans_done below for the final lookup.)
    const esp_timer_create_args_t tick_args = {
        .callback = lvgl_tick_cb, .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 2000));
    return ESP_OK;
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "Starting EV gauge");
    ESP_ERROR_CHECK(display_init());

    // UI is updated from the same task that runs LVGL, avoiding concurrent LVGL calls.
    // draw screen here
    printf("making loading screen\n");
    gauge_loading_screen();
    printf("loading Screen Set\n");
    printf("init\n");
    gauge_ui_init();
    printf("init complete\n");
    BaseType_t task_created = xTaskCreate(
        lvgl_task,
        "lvgl_task",
        4096,
        NULL,
        5,
        NULL
    );
    
    
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_ERROR_CHECK(can_receiver_start());
    vTaskDelay(pdMS_TO_TICKS(1500));
    request_can(0x02, 0x02, 0x00, 0x00, 0x02, 0x00);

    

    

    
    

    gauge_data_t data;
    
    gauge_data_get(&data);
    printf("got data\n");
    
    
    printf("updating ui\n");
    
    
    printf("setup complete\n");
    s_setup_done = true;



    // if (task_created != pdPASS) {
    //     ESP_LOGE(TAG, "Failed to create LVGL task");
    //     abort();
    // }


    
    int64_t last_request_us = 0;
    while (1) {
        // add logic about getting data and resending can request if needed
        // if no data for 2s first request data again. if no data after 5s then update values to '-'
        gauge_data_get(&data);
        int64_t now_us = esp_timer_get_time();
        int64_t elapsed = (now_us -  data.last_update_us) / 1000;
        
        
        if (elapsed > 2000 && (now_us - last_request_us) > 1000000) {
            request_can(0x02, 0x02, 0x00, 0x00, 0x02, 0x00);
            last_request_us = now_us;
            printf("requesting data\n");
        }

        if (elapsed > 5000) {
            expired_data = true;
            printf("data expired\n");
        } else {
            expired_data = false;
        }

        
        vTaskDelay(pdMS_TO_TICKS(100));

        

    }
}
