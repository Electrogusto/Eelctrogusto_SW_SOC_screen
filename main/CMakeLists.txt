idf_component_register(
    SRCS "main.c" "gauge_ui.c" "can_receiver.c" "gauge_background.c" "logo.c" "loading_screen.c" "Highway_Gothic_180.c" "Highway_Gothic_90.c" "Highway_Gothic_40.c" "Highway_Gothic_150.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_lcd esp_timer lvgl
)