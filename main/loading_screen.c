#include "lvgl.h"
#include "loading_screen.h"
#include "logo.h"

static lv_obj_t *loading_screen;

void loading_screen_create(void)
{
    loading_screen = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(
        loading_screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(
        loading_screen, LV_OPA_COVER, 0);

    lv_obj_t *logo_img = lv_image_create(loading_screen);
    lv_image_set_src(logo_img, &logo);

    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, 0);

    lv_screen_load(loading_screen);
}

void loading_screen_show_gauge(void)
{
    /* Replace this with your actual gauge-screen function. */
}