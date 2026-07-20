#pragma message("USING MY CUSTOM USER_SETUP.H")

#ifndef USER_SETUP_H
#define USER_SETUP_H

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17

#define TOUCH_CS 25

#define TFT_BL 4
#define TFT_BACKLIGHT_ON HIGH
#define TFT_RGB
#define TFT_INVERSION_OFF

#define SPI_FREQUENCY 10000000
#define SPI_READ_FREQUENCY 10000000
#define SPI_TOUCH_FREQUENCY 1000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#endif