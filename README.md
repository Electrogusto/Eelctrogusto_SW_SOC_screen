# ESP-IDF EV Gauge — Waveshare ESP32-S3-Touch-LCD-2.8 V1

Initial LVGL 9 + ESP-IDF project scaffold for 320x240 landscape. Uses ST7789 over SPI and ESP32 TWAI CAN.

## Important hardware notes

* Board revision: V1, ST7789 display and CST328 touch. Touch is not initialised in this first version because the gauge does not need touch input yet.
* Display pins from Waveshare's published pinout: MOSI 45, SCLK 40, CS 42, DC 41, reset 39, backlight 5.
* CAN GPIO17 TX and GPIO18 RX are example choices only. Confirm these are physically wired to your external CAN transceiver and are free on your board.
* CAN bitrate is configured to 500 kbit/s. Change `CAN_BITRATE` and the timing config if your vehicle uses a different bitrate. This initial example currently uses the 500 kbit/s timing macro.
* Connect CAN transceiver CANH/CANL to the bus, share ground as appropriate, and ensure the bus has correct termination. Do not connect vehicle CANH/CANL directly to ESP32 GPIO.
* Do not enable the sample CAN decoder until you replace CAN ID, byte mapping, scale and gear mapping with your real vehicle protocol.

## Dependencies

Open the ESP-IDF terminal in this folder and run:

```sh
idf.py set-target esp32s3
idf.py reconfigure
idf.py build
idf.py -p PORT flash monitor
```

The Component Manager reads `main/idf_component.yml` and downloads LVGL 9 and the ST7789 panel component. Internet access is required on the first configure/build.

## Files to customise

* `main/gauge_config.h`: fonts, colours, battery energy, consumption, pins and background toggle.
* `main/gauge_ui.c`: position and size of the widgets. Each widget is only changed when its value changes.
* `main/gauge_background.c`: replace the embedded RGB565 image data with an image converted for LVGL 9, or set `GAUGE_USE_BACKGROUND_IMAGE` to `0`.
* `main/can_receiver.c`: implement the real CAN frame decoder.

## Range formula

`range_km = (SOC / 100) * usable_battery_kWh * 100 / consumption_kWh_per_100km`

Defaults (30 kWh usable, 15 kWh/100 km) are examples only; set these to your EV.

## CAN decoder placeholder

The example decoder is disabled by default (`CAN_DECODER_ENABLED 0`). If enabled, it treats CAN ID 0x123 as:
byte 0 = SOC %, bytes 1-2 = signed little-endian power in 0.1 kW, byte 3 = gear (0=R, 1=N, 2=D). This is not a standard EV CAN mapping; it is just an example format. Replace it with your actual sender's frame format.

## Background image format

The included background is a compiled-in RGB565 LVGL image at 320x240. To use a new design, convert a 320x240 image to RGB565 C source with the LVGL image converter and replace `gauge_background.c`'s descriptor/data. Keep the descriptor symbol named `gauge_background` and the header dimensions 320x240.

## First build caveat

This is a starter scaffold and has not been built against your exact ESP-IDF installation/managed-component versions. The ESP-IDF LCD API and LVGL component versions can differ; resolve any version-specific compile diagnostics before connecting it to a live vehicle bus. Validate the display and CAN transceiver on a bench first.
