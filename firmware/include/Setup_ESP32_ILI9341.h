#ifndef SETUP_ESP32_ILI9341_H
#define SETUP_ESP32_ILI9341_H

// Display driver
#define ILI9341_DRIVER

// Display size
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// SPI pins
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

// Control pins
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17

// Touch controller
#define TOUCH_CS 25

// Backlight
#define TFT_BL 4

// SPI speeds
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000

// Fonts
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// Smooth fonts
#define SMOOTH_FONT

#endif