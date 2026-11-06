#include "gauge_ui.h"
#include "gauge_config.h"
#include "gauge_background.h"
#include "loading_screen.h"
#include "lvgl.h"
#include <stdio.h>
#include <math.h>
#include "logo.h"

static lv_obj_t *soc_value,  *charge_value, *charge_unit, *power_bar, *soc_unit, *charge_soc_value, *charge_soc_unit;
static gauge_data_t previous_driving = { .soc_percent = -1, .power_kw = -9999.0f, .charge_kw = -9999.0f, .charge_state = 10, .plug_state = 101, .is_charging = false};
static gauge_data_t previous_charging = { .soc_percent = -1, .power_kw = -9999.0f, .charge_kw = -9999.0f, .charge_state = 10, .plug_state = 101, .is_charging = true};

static lv_obj_t *driving_screen;
static lv_obj_t *loading_screen_obj;
static lv_obj_t *charging_screen;

static void set_text_colour(lv_obj_t *obj, uint32_t colour) {
    lv_obj_set_style_text_color(obj, lv_color_hex(colour), 0);
}

void gauge_ui_init(void)
{
    driving_screen = lv_obj_create(NULL);
    lv_obj_t *d_screen = driving_screen;
    lv_obj_set_style_bg_color(d_screen, lv_color_hex(COL_BACKGROUND), 0);
    lv_obj_set_style_bg_opa(d_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(d_screen, 0, 0);

    charging_screen = lv_obj_create(NULL);
    lv_obj_t *c_screen = charging_screen;
    lv_obj_set_style_bg_color(c_screen, lv_color_hex(COL_BACKGROUND), 0);
    lv_obj_set_style_bg_opa(c_screen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(c_screen, 0, 0);



#if GAUGE_USE_BACKGROUND_IMAGE
    lv_obj_t *bg = lv_image_create(d_screen);
    lv_image_set_src(bg, &gauge_background);
    lv_obj_set_size(bg, GAUGE_WIDTH, GAUGE_HEIGHT);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_move_background(bg);
#endif

    // drivng screen
    soc_value = lv_label_create(d_screen);
    lv_label_set_text(soc_value, "-");
    lv_obj_set_style_text_font(soc_value, FONT_LARGE, 0);
    set_text_colour(soc_value, COL_SOC_LOW);
    lv_obj_align(soc_value, LV_ALIGN_CENTER, SOC_OFFSET_X, SOC_OFFSET_Y);
    lv_obj_set_style_text_letter_space(soc_value, -6, 0);


    soc_unit = lv_label_create(d_screen);
    lv_label_set_text(soc_unit, "%");
    lv_obj_set_style_text_font(soc_unit, FONT_MEDIUM, 0);
    set_text_colour(soc_unit, COL_SOC_LOW);
    lv_obj_align_to(soc_unit, soc_value, LV_ALIGN_BOTTOM_RIGHT, SOC_UNIT_DX, SOC_UNIT_DY);


    power_bar = lv_bar_create(d_screen);
    int power_bar_height = 12;
    lv_obj_set_size(power_bar, GAUGE_WIDTH - 40, power_bar_height);
    lv_obj_set_pos(power_bar, 20, GAUGE_HEIGHT - power_bar_height - 20);
    lv_bar_set_mode(power_bar, LV_BAR_MODE_SYMMETRICAL);
    lv_bar_set_start_value(power_bar, 0, LV_ANIM_OFF);
    lv_bar_set_range(power_bar, MAX_REGEN, MAX_TRACTION);
    lv_bar_set_value(power_bar, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(power_bar, lv_color_hex(0x333333), LV_PART_MAIN);         
    lv_obj_set_style_bg_opa(power_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(power_bar, lv_color_hex(COL_TRACTION), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(power_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(power_bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(power_bar, 0, LV_PART_MAIN);

    

    // charging screen
    charge_soc_value = lv_label_create(c_screen);
    lv_label_set_text(charge_soc_value, "-");
    lv_obj_set_style_text_font(charge_soc_value, FONT_LARGE, 0);
    set_text_colour(charge_soc_value, COL_SOC_LOW);
    lv_obj_align(charge_soc_value, LV_ALIGN_BOTTOM_LEFT, CHARGE_SOC_OFFSET_X, CHARGE_SOC_OFFSET_Y);
    lv_obj_set_style_text_letter_space(charge_soc_value, -10, 0);

    charge_soc_unit = lv_label_create(c_screen);
    lv_label_set_text(charge_soc_unit, "%");
    lv_obj_set_style_text_font(charge_soc_unit, FONT_MEDIUM, 0);
    set_text_colour(charge_soc_unit, COL_SOC_LOW);
    lv_obj_align_to(charge_soc_unit, charge_soc_value, LV_ALIGN_BOTTOM_RIGHT, CHARGE_SOC_UNIT_DX, CHARGE_SOC_UNIT_DY);


    charge_value = lv_label_create(c_screen);
    lv_label_set_text(charge_value, "-");
    lv_obj_set_style_text_font(charge_value, FONT_MEDIUM, 0);
    set_text_colour(charge_value, COL_CHARGE);
    // lv_obj_align(charge_value, LV_ALIGN_TOP_RIGHT, CHARGE_VAL_OFFSET_X + CHARGE_SEPERATE_X, CHARGE_VAL_OFFSET_Y);
    lv_obj_align(charge_value, LV_ALIGN_TOP_RIGHT, -60, 20);
    lv_obj_set_style_text_letter_space(charge_value, -6, 0); 

    charge_unit = lv_label_create(c_screen);
    lv_label_set_text(charge_unit, "kW");
    lv_obj_set_style_text_font(charge_unit, FONT_SMALL, 0);
    set_text_colour(charge_unit, COL_CHARGE);
    lv_obj_align_to(charge_unit, charge_value, LV_ALIGN_BOTTOM_RIGHT, CHARGE_UNIT_DX, CHARGE_UNIT_DY);
}


void gauge_ui_update(const gauge_data_t *d) {
    if (d->is_charging) {
        gauge_ui_update_charging(d);
        lv_screen_load(charging_screen);
    } else {
        gauge_ui_update_driving(d);
        lv_screen_load(driving_screen);
    }

}


void gauge_ui_update_driving(const gauge_data_t *d) {
    char text[32];

    // Only update widgets whose underlying values have changed.
    if (abs(d->soc_percent - previous_driving.soc_percent) >= 0.1f) {
        uint32_t soc_colour = COL_SOC_LOW;
        if (d->soc_percent < 0) {
            snprintf(text, sizeof(text), "-");
            lv_label_set_text(soc_value, text);
        } else {
            snprintf(text, sizeof(text), "%.0d", d->soc_percent);
            
            if (d->soc_percent > SOC_MED) {
                soc_colour = COL_SOC_HIGH;
            } else if (d->soc_percent > SOC_LOW) {
                soc_colour = COL_SOC_MED;
            }
        }
        lv_label_set_text(soc_value, text);
        set_text_colour(soc_value, soc_colour);
        previous_driving.soc_percent = d->soc_percent;
        lv_obj_align(soc_value, LV_ALIGN_CENTER, SOC_OFFSET_X, SOC_OFFSET_Y);
        lv_obj_update_layout(soc_value);
        lv_obj_align_to(soc_unit, soc_value, LV_ALIGN_BOTTOM_RIGHT, SOC_UNIT_DX, SOC_UNIT_DY);
        set_text_colour(soc_unit, soc_colour);
    }

    if (fabsf(d->power_kw - previous_driving.power_kw) >= 0.1f) {
        // snprintf(text, sizeof(text), "%+.0fkW", d->power_kw);
        // lv_label_set_text(power_value, text);
        // set_text_colour(power_value, d->power_kw < -0.1f ? COL_ACCENT : COL_BLUE);
        previous_driving.power_kw = d->power_kw;
        lv_bar_set_value(power_bar, (int)(d->power_kw >= 0 ? d->power_kw * TRACTION_SCALER : d->power_kw) , LV_ANIM_ON);
        lv_obj_set_style_anim_duration(power_bar, POWER_BAR_ANIM_TIME, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(power_bar, lv_color_hex(d->power_kw < 0.1f ? COL_REGEN : COL_TRACTION), LV_PART_INDICATOR);
    }
}


void gauge_ui_update_charging(const gauge_data_t *d) {
    char soc_text[32];
    char charge_text[32];

    // Only update widgets whose underlying values have changed.
    // soc
    if (abs(d->soc_percent - previous_charging.soc_percent) >= 0.1f) {
        uint32_t soc_colour = COL_SOC_LOW;
        if (d->soc_percent < 0) {
            snprintf(soc_text, sizeof(soc_text), "-");
            lv_label_set_text(soc_value, soc_text);
        } else {
            snprintf(soc_text, sizeof(soc_text), "%.0d", d->soc_percent);
            
            if (d->soc_percent > SOC_MED) {
                soc_colour = COL_SOC_HIGH;
            } else if (d->soc_percent > SOC_LOW) {
                soc_colour = COL_SOC_MED;
            }
        }
        lv_label_set_text(charge_soc_value, soc_text);
        set_text_colour(charge_soc_value, soc_colour);
        previous_charging.soc_percent = d->soc_percent;
        lv_obj_align(charge_soc_value, LV_ALIGN_BOTTOM_LEFT, CHARGE_SOC_OFFSET_X, CHARGE_SOC_OFFSET_Y);
        lv_obj_update_layout(charge_soc_value);
        lv_obj_align_to(charge_soc_unit, charge_soc_value, LV_ALIGN_BOTTOM_RIGHT, CHARGE_SOC_UNIT_DX, CHARGE_SOC_UNIT_DY);
        set_text_colour(charge_soc_unit, soc_colour);
    }

    // charge kw
    if (fabsf(d->power_kw - previous_charging.power_kw) >= 0.1f) {

        snprintf(charge_text, sizeof(charge_text), "%.f", d->power_kw * -1);

        lv_label_set_text(charge_value, charge_text);
        previous_charging.power_kw = d->power_kw;
        lv_obj_align(charge_value, LV_ALIGN_TOP_RIGHT, -60, 20);
        lv_obj_update_layout(charge_value);
        lv_obj_align_to(charge_unit, charge_value, LV_ALIGN_BOTTOM_RIGHT, CHARGE_UNIT_DX, CHARGE_UNIT_DY);
    }


    // old
    // if (fabsf(d->charge_kw - previous_charging.charge_kw) >= 0.1f) {
    //     if (d->charge_kw < 0) {
    //         snprintf(charge_text, sizeof(charge_text), "-");
    //     } else {
    //         snprintf(charge_text, sizeof(charge_text), "%.f", d->charge_kw);
    //     }
    //     lv_label_set_text(charge_value, charge_text);
    //     previous_charging.charge_kw = d->charge_kw;
    //     lv_obj_align(charge_value, LV_ALIGN_TOP_RIGHT, -60, 20);
    //     lv_obj_update_layout(charge_value);
    //     lv_obj_align_to(charge_unit, charge_value, LV_ALIGN_BOTTOM_RIGHT, CHARGE_UNIT_DX, CHARGE_UNIT_DY);
    // }
}


void gauge_loading_screen(void) {
    printf("creating loading screen \n");
    loading_screen_obj = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(
        loading_screen_obj, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(
        loading_screen_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(
        loading_screen_obj, 0, 0);
    lv_obj_set_style_border_width(
        loading_screen_obj, 0, 0);

    lv_obj_t *logo_img = lv_image_create(loading_screen_obj);
    lv_image_set_src(logo_img, &logo);
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 3, 0);

    lv_screen_load(loading_screen_obj);

    printf("finished loading screen\n");

}

void gauge_ui_show(void) {
    if (driving_screen != NULL) {
        lv_screen_load(driving_screen);
    }
}
