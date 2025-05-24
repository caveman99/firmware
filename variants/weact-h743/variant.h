#ifndef _VARIANT_WEACT_H743_
#define _VARIANT_WEACT_H743_

#define USE_ST7789

#define ST7789_NSS PE11
#define ST7789_RS PE13  // DC
#define ST7789_SDA PE14 // MOSI
#define ST7789_SCK PE12
#define ST7789_RESET -1
#define ST7789_MISO -1
#define ST7789_BUSY -1
#define VTFT_LEDA PE10
#define TFT_BACKLIGHT_ON LOW
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define TFT_HEIGHT 135
#define TFT_WIDTH 240
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0

#define HAS_BUTTON 1
#define BUTTON_PIN USER_BTN

#define LED_PIN LED_BUILTIN
#define LED_STATE_ON 1

// I2C definitions
#define HAS_WIRE 1
#define WIRE_INTERFACES_COUNT 1
#define I2C_SDA PIN_WIRE_SDA
#define I2C_SCL PIN_WIRE_SCL

#define HAS_SCREEN 1

// #define HAS_SDCARD // Have SPI interface SD card slot. Use card in SPI mode
// #define SDX_D0 PC8 // SDMMC1_D0
// #define SDX_D1 PC9 // SDMMC1_D1
// #define SDX_D2 PC10 // SDMMC1_D2
// #define SDX_D3 PC11 // SDMMC1_D3
// #define SDX_CK PC12 // SDMMC1_CK
// #define SDX_CMD PD2 // SDMMC1_CMD
// #define SD_DETECT_PIN PD4

#endif
