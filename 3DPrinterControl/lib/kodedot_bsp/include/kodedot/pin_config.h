#pragma once
/* ---------- Buttons ---------- */
#define BUTTON_TOP            0   // GPIO_NUM_0
// Bottom button is provided by the IO expander
#define BUTTON_BOTTOM         EXPANDER_BUTTON_BOTTOM

/* ---------- NeoPixel LED ---------- */
#define LED_STRIP_PIN         4
#define LED_STRIP_NUM_LEDS    1
#define LED_STRIP_RMT_RES_HZ  10000000  // 10 MHz
// Color order and timing for WS2812B
#ifndef LED_STRIP_COLOR_ORDER
#define LED_STRIP_COLOR_ORDER  NEO_GRB
#endif
#ifndef LED_STRIP_TIMING
#define LED_STRIP_TIMING       NEO_KHZ800
#endif

/* ---------- LCD Panel ---------- */
#define LCD_WIDTH             410
#define LCD_HEIGHT            502
#define LCD_SPI_HOST          SPI3_HOST
#define LCD_PIXEL_CLK_HZ      40000000    // 40 MHz
#define LCD_CMD_BITS          8
#define LCD_PARAM_BITS        8
#define LCD_COLOR_SPACE       ESP_LCD_COLOR_SPACE_RGB
#define LCD_BITS_PER_PIXEL    16
#define LCD_DRAW_BUFF_DOUBLE  1
// Use full-height buffer (PSRAM available)
#define LCD_DRAW_BUFF_HEIGHT  LCD_HEIGHT

// LCD pins (QSPI)
#define LCD_SCLK              17
#define LCD_SDIO0             15
#define LCD_SDIO1             14
#define LCD_SDIO2             16
#define LCD_SDIO3             10
#define LCD_RST                8
#define LCD_CS                 9
#define LCD_EN               -1  // no dedicated enable pin

/* ---------- Touch / IO Expander ---------- */
#define TOUCH_I2C_NUM         0
#define TOUCH_I2C_SCL         47
#define TOUCH_I2C_SDA         48
#define TOUCH_INT            -1
#define TOUCH_RST            -1

#define IOEXP_I2C_NUM         TOUCH_I2C_NUM
#define IOEXP_I2C_SCL         TOUCH_I2C_SCL
#define IOEXP_I2C_SDA         TOUCH_I2C_SDA
#define IOEXP_I2C_ADDR        0x20   // TCA9555 address 000
#define IOEXP_INT_PIN         18

/* ---------- SD-Card (SDMMC) ---------- */
// Host and frequency
#define SD_SDMMC_HOST         SDMMC_HOST_SLOT_1
#define SD_MAX_FREQ_KHZ       50000
// SDMMC pins (1-bit width)
#define SD_PIN_CMD            5
#define SD_PIN_CLK            6
#define SD_PIN_D0             7
// Card detect via IO expander
#define SD_DETECT_PIN         EXPANDER_SD_CD
// Mount parameters
#define SD_MOUNT_POINT        "/sdcard"
#define SD_MAX_FILES          5
#define SD_FORMAT_IF_FAIL     0

/* ---------- Microphone (I2S) ---------- */
// Digital MEMS Mic connected via I2S (see docs: Microphone)
#define MIC_I2S_SCK           38   // SCK / BCLK
#define MIC_I2S_WS            45   // WS  / LRCLK
#define MIC_I2S_DIN           21   // SD  / DATA

/* ---------- Sensors ---------- */
#define MAX17048_I2C_ADDRESS  0x36
#define BQ25896_I2C_ADDRESS   0x6A

/* ---------- IO Expander pin map ---------- */
// These defines require including <TCA9555.h> in the source file that uses them
#define EXPANDER_MAG_INT        0x00  // P00 - Magnetometer interrupt
#define EXPANDER_RTC_INTB       0x01  // P01 - RTC INTB
#define EXPANDER_RTC_INTA       0x02  // P02 - RTC INTA
#define EXPANDER_SPK_SHUTDOWN   0x03  // P03 - Speaker shutdown
#define EXPANDER_PWR_PHRL       0x04  // P04 - Power PHRL
#define EXPANDER_FG_ALRT        0x05  // P05 - Fuel gauge alert
#define EXPANDER_PAD_TOP        0x06  // P06 - D-pad TOP (external pull-up)
#define EXPANDER_PAD_LEFT       0x07  // P07 - D-pad LEFT (external pull-up)
#define EXPANDER_PAD_BOTTOM     8     // P10 - D-pad BOTTOM (external pull-up)
#define EXPANDER_BUTTON_BOTTOM  9     // P11 - Bottom button (external pull-up)
#define EXPANDER_PWR_INT        10    // P12 - Power INT
#define EXPANDER_PAD_RIGHT      11    // P13 - D-pad RIGHT (external pull-up)
#define EXPANDER_SD_CD          14    // P16 - SD card detect

/* ---------- NeoPixel convenience ---------- */
#define NEO_PIXEL_PIN    LED_STRIP_PIN
#define NEO_PIXEL_COUNT  LED_STRIP_NUM_LEDS


