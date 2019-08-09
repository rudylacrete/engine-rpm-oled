# Abstract

This simple project use an atmega328 to interact with an ELM327 through UART and display basic informations on an OLED screen (SSD1306).

# Wiring

The oled screen is using I2C bus. It requires the following wiring:

|SSD1306 pin | atmega pin|
|-----|-----|
| 5V  | 5V  |
| GND | GND |
| SDA | A4  |
| SCL | A5  |

# How to compile

Update SSD1306 library to use the correct size. Uncomment the following line in `libdeps/uno/Adafruit SSD1306 128x64_ID1523/Adafruit_SSD1306.h`:

```c
  #define SSD1306_128_64
  //  #define SSD1306_128_32
  //   #define SSD1306_96_16
```

Then install dependencies and flash the board:

```sh
# install deps
platformio lib install
# compile and flash board
platformio run -t upload
```
