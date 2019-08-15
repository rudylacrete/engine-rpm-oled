#include <Arduino.h>
#include "elm327.h"
#include <Wire.h>
#include <math.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
This program is short to run on atmega328 because of the big memory footprint
of adafruit ssd lib. If the program takes more than 93% of RAM at compilation, there
is a lot of chances that the software will bug (restart, freeze, etc)
because it's running out of space for dynamic mem allocation.
To deal with that, I've used PROGMEM instruction whenever possible (readonly var)
to put things into flash and save RAM. I'm also using the string helper F() which
does the same for static strings.
*/

const int OLED_HEIGHT PROGMEM = 64;
const int OLED_WIDTH PROGMEM = 128;

const int SEGMENT_HEIGHT PROGMEM = 16;
const int TEXT_SIZE_SMALL PROGMEM = 1;
const int TEXT_SIZE_LARGE PROGMEM = 2;
const int ONE_K PROGMEM = 100;

const uint16_t DIAL_CENTER_X PROGMEM = OLED_WIDTH / 2;
const uint16_t DIAL_RADIUS PROGMEM = (OLED_HEIGHT - SEGMENT_HEIGHT) - 1;
const uint16_t DIAL_CENTER_Y PROGMEM = OLED_HEIGHT - 1;
const uint16_t INDICATOR_LENGTH PROGMEM = DIAL_RADIUS - 5;
const uint16_t INDICATOR_WIDTH PROGMEM = 5;
const uint16_t LABEL_RADIUS PROGMEM = DIAL_RADIUS - 18;
const int DIAL_LABEL_Y_OFFSET PROGMEM = 6;
const int DIAL_LABEL_X_OFFSET PROGMEM = 4;

const uint16_t MAJOR_TICKS[] PROGMEM= { 0, 1000, 2000, 3000, 4000, 5000 };
const int MAJOR_TICK_COUNT PROGMEM = sizeof(MAJOR_TICKS) / sizeof(MAJOR_TICKS[0]);
const int MAJOR_TICK_LENGTH PROGMEM = 11;
const uint16_t MINOR_TICKS[] PROGMEM = { 500, 1500, 2500, 3500, 4500 };
const int MINOR_TICK_COUNT PROGMEM = sizeof(MINOR_TICKS) / sizeof(MINOR_TICKS[0]);
const int MINOR_TICK_LENGTH PROGMEM = 5;

const uint16_t DIAL_MAX_RPM PROGMEM = MAJOR_TICKS[MAJOR_TICK_COUNT-1];
const uint8_t HALF_CIRCLE_DEGREES PROGMEM = 180;
const float PI_RADIANS PROGMEM = PI/HALF_CIRCLE_DEGREES;

void drawTicks(const uint16_t*, int, int);
float getPercentMaxRpm(int);
float getCircleXWithLengthAndAngle(uint16_t, float);
float getCircleYWithLengthAndAngle(uint16_t, float);
void drawMajorTickLabels(void);
void drawIndicatorHand(int);
void drawRpm(int);
void drawCoolantTemp(int);
void displayInfo(const __FlashStringHelper*);

static error_code_t error;

Adafruit_SSD1306 disp(4);
ObdReader elm({
  .rxPin = 8,
  .txPin = 9
});
static int rpm = 0;

void setup() {
  Serial.begin(9600);
  disp.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // F() is an helper used to store string into flash instead of RAM
  displayInfo(F("Setting up"));
  elm.enable_debug(true);
  error = elm.setup();
  elm.enable_debug(false);
  switch (error)
  {
    case RESET_ERROR:
      displayInfo(F("Reset failed"));
      exit(1);
      break;

    case ECHO_OFF_ERROR:
      displayInfo(F("Echo off error"));
      exit(1);
      break;

    case GET_VOLTAGE_ERROR:
      displayInfo(F("Voltage error"));
      exit(1);
      break;

    case OBD_NOT_CONNECTED:
      displayInfo(F("OBD not con"));
      exit(1);
      break;

    case SELECT_PROTOCOL_ERROR:
      displayInfo(F("PROTO AUTO err"));
      exit(1);
      break;

    case PID00_ERROR:
      displayInfo(F("PID00 err"));
      exit(1);
      break;

    case PID20_ERROR:
      displayInfo(F("PID20 err"));
      exit(1);
      break;

    case PID40_ERROR:
      displayInfo(F("PID40 err"));
      exit(1);
      break;

    case NO_ERROR:
      displayInfo(F("Setup done"));
      Serial.println(F("Setup done"));
      delay(1000);
      break;

    default:
      displayInfo(F("UKN error"));
      Serial.println(F("Unknow error occured"));
      exit(1);
      break;
  }
}

void loop() {
  delay(50);
  rpm = elm.getRpm();
  if(rpm != 0) {
    disp.clearDisplay();
    drawRpm(rpm);
    disp.display();
  }
}

void displayInfo(const __FlashStringHelper* text) {
  disp.clearDisplay();
  disp.setCursor(0, OLED_HEIGHT/2);
  disp.setTextSize(TEXT_SIZE_LARGE);
  disp.setTextColor(WHITE);
  disp.print(text);
  disp.display();
}

void drawRpm(int rpm) {
  disp.drawCircle(DIAL_CENTER_X, DIAL_CENTER_Y, DIAL_RADIUS, WHITE);
  drawTicks(MAJOR_TICKS, MAJOR_TICK_COUNT, MAJOR_TICK_LENGTH);
  drawTicks(MINOR_TICKS, MINOR_TICK_COUNT, MINOR_TICK_LENGTH);
  drawMajorTickLabels();
  drawIndicatorHand(rpm);
}

void drawTicks(const uint16_t ticks[], int tick_count, int tick_length) {
  for (int tick_index = 0; tick_index < tick_count; tick_index++) {
    // this array is stored in flash so need to use pgm_read_word to get value
		int rpm_tick_value = (int)pgm_read_word(ticks + tick_index);
		float tick_angle = (HALF_CIRCLE_DEGREES * getPercentMaxRpm(rpm_tick_value)) + HALF_CIRCLE_DEGREES;
		uint16_t dial_x = getCircleXWithLengthAndAngle(DIAL_RADIUS - 1, tick_angle);
		uint16_t dial_y = getCircleYWithLengthAndAngle(DIAL_RADIUS - 1, tick_angle);
		uint16_t tick_x = getCircleXWithLengthAndAngle(DIAL_RADIUS - tick_length, tick_angle);
		uint16_t tick_y = getCircleYWithLengthAndAngle(DIAL_RADIUS - tick_length, tick_angle);
		disp.drawLine(dial_x, dial_y, tick_x, tick_y, WHITE);
	}
}

float getPercentMaxRpm(int value) {
	float ret_value = (value * 1.0)/(DIAL_MAX_RPM * 1.0);
	return ret_value;
}

float getCircleXWithLengthAndAngle(uint16_t radius, float angle) {
	return DIAL_CENTER_X + radius * cos(angle*PI_RADIANS);
};

float getCircleYWithLengthAndAngle(uint16_t radius, float angle) {
	return DIAL_CENTER_Y + radius * sin(angle*PI_RADIANS);
};

void drawMajorTickLabels() {
	disp.setTextSize(TEXT_SIZE_SMALL);
  disp.setTextColor(WHITE);
	for (int label_index = 0; label_index < MAJOR_TICK_COUNT; label_index++) {
    // draw only half labels
    if(label_index% 2==0) continue;
    // this array is stored in flash so need to use pgm_read_word to get value
		int rpm_tick_value = pgm_read_word(MAJOR_TICKS + label_index);
		float tick_angle = (HALF_CIRCLE_DEGREES	* getPercentMaxRpm(rpm_tick_value)) + HALF_CIRCLE_DEGREES;
		uint16_t dial_x = getCircleXWithLengthAndAngle(LABEL_RADIUS, tick_angle);
		uint16_t dial_y = getCircleYWithLengthAndAngle(LABEL_RADIUS, tick_angle);
		disp.setCursor(dial_x - DIAL_LABEL_X_OFFSET, dial_y - DIAL_LABEL_Y_OFFSET);
		int label_value = rpm_tick_value / ONE_K;
		disp.print(label_value);
	}
}

void drawIndicatorHand(int rpm_value) {
  float indicator_angle = (HALF_CIRCLE_DEGREES * getPercentMaxRpm(rpm_value)) + HALF_CIRCLE_DEGREES;
  uint16_t indicator_top_x = getCircleXWithLengthAndAngle(INDICATOR_LENGTH, indicator_angle);
  uint16_t indicator_top_y = getCircleYWithLengthAndAngle(INDICATOR_LENGTH, indicator_angle);

	disp.drawTriangle(DIAL_CENTER_X - INDICATOR_WIDTH / 2,
                    DIAL_CENTER_Y,DIAL_CENTER_X + INDICATOR_WIDTH / 2,
                    DIAL_CENTER_Y,
                    indicator_top_x,
                    indicator_top_y,
                    WHITE);
}

void drawCoolantTemp(int temp) {
  disp.setCursor(0, 0);

  disp.setTextSize(TEXT_SIZE_LARGE);
  disp.print(F("TEMP: "));
  disp.setTextSize(TEXT_SIZE_SMALL);
  disp.print(temp);
  disp.drawCircle(86 + (temp > 100 ? 7 : 0), 2, 1, WHITE);
}
