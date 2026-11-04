#pragma once
#include "lvgl.h"


// ---------------- USER CUSTOMISATION ----------------
#define GAUGE_WIDTH                 320
#define GAUGE_HEIGHT                240

// Board V1: Waveshare ESP32-S3-Touch-LCD-2.8
#define PIN_LCD_MOSI                45
#define PIN_LCD_SCLK                40
#define PIN_LCD_CS                  42
#define PIN_LCD_DC                  41
#define PIN_LCD_RST                 39
#define PIN_LCD_BACKLIGHT            5

// CAN transceiver: change these to your actual wiring.
// Avoid GPIOs already used by the board peripherals above.
#define PIN_CAN_TX                  18
#define PIN_CAN_RX                  15
#define CAN_BITRATE                 250000

// Set this to your battery's usable energy and typical consumption.
#define BATTERY_USABLE_KWH          30.0f
#define CONSUMPTION_KWH_PER_100KM   15.0f

// LVGL built-in fonts; change these centrally.
LV_FONT_DECLARE(Highway_Gothic_180);
LV_FONT_DECLARE(Highway_Gothic_90);
LV_FONT_DECLARE(Highway_Gothic_40);
LV_FONT_DECLARE(Highway_Gothic_150);
#define FONT_SMALL                  (&Highway_Gothic_40)
#define FONT_MEDIUM                 (&Highway_Gothic_90)
#define FONT_LARGE                  (&Highway_Gothic_180)
#define FONT_MEDIUM_BIG             (&Highway_Gothic_150)


// Theme colours (RGB hex). Change freely.
#define COL_BACKGROUND              0x000000
#define COL_PANEL                   0x151E2B
#define COL_TEXT                    0xF2F5F8
#define COL_MUTED                   0x8795A8
#define COL_ACCENT                  0x4CE0A0
#define COL_BLUE                    0x52B8FF
#define COL_AMBER                   0xF15d2b
#define COL_RED                     0xFF0000
#define COL_TRACTION                0xFF0000
#define COL_REGEN                   0x2E8942
#define COL_SOC_HIGH                0x2E8942
#define COL_SOC_MED                 0xF15d2b
// #define COL_SOC_LOW                 0xED2024 // nick chose guess 1
#define COL_SOC_LOW                 0xFF0000
#define COL_CHARGE                  0x2E8942


// Background: 0 = plain theme; 1 = use embedded gauge_background image.
#define GAUGE_USE_BACKGROUND_IMAGE  0

// Set true only after confirming the vehicle's CAN IDs and scaling.
#define CAN_DECODER_ENABLED         1

// max regen and teraction in KW
#define MAX_TRACTION                50
#define MAX_REGEN                   -50 // should be negative
#define TRACTION_SCALER             0.5
#define POWER_BAR_ANIM_TIME         1000 // in ms

// SOC ranges (in percentage)
#define SOC_MED 40
#define SOC_LOW 20

#define SOC_UNIT_DX 40
#define SOC_UNIT_DY 0

#define SOC_OFFSET_X -13
#define SOC_OFFSET_Y -10

#define CHARGE_SEPERATE_X 60

#define CHARGE_SOC_OFFSET_X 0
#define CHARGE_SOC_OFFSET_Y -10

#define CHARGE_VAL_OFFSET_X 10
#define CHARGE_VAL_OFFSET_Y -50

#define CHARGE_SOC_UNIT_DX 40
#define CHARGE_SOC_UNIT_DY 0

#define CHARGE_UNIT_DX 50
#define CHARGE_UNIT_DY 0