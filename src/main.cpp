#include <Arduino.h>
#include <SoftwareSerial.h>
#include "elm327.h"
#include <Wire.h>
#include <math.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int OLED_HEIGHT = 64;
const int OLED_WIDTH = 128;

const int SEGMENT_HEIGHT = 16;
const int TEXT_SIZE_SMALL = 1;
const int TEXT_SIZE_LARGE = 2;
const int ONE_K = 100;

const uint16_t DIAL_CENTER_X = OLED_WIDTH / 2;
const uint16_t DIAL_RADIUS = (OLED_HEIGHT - SEGMENT_HEIGHT) - 1;
const uint16_t DIAL_CENTER_Y = OLED_HEIGHT - 1;
const uint16_t INDICATOR_LENGTH = DIAL_RADIUS - 5;
const uint16_t INDICATOR_WIDTH = 5;
const uint16_t LABEL_RADIUS = DIAL_RADIUS - 18;
const int DIAL_LABEL_Y_OFFSET = 6;
const int DIAL_LABEL_X_OFFSET = 4;

const long MAJOR_TICKS[] = { 0, 1000, 2000, 3000, 4000, 5000 };
const int MAJOR_TICK_COUNT = sizeof(MAJOR_TICKS) / sizeof(MAJOR_TICKS[0]);
const int MAJOR_TICK_LENGTH = 11;
const long MINOR_TICKS[] = { 500, 1500, 2500, 3500, 4500 };
const int MINOR_TICK_COUNT = sizeof(MINOR_TICKS) / sizeof(MINOR_TICKS[0]);
const int MINOR_TICK_LENGTH = 5;

const uint16_t DIAL_MAX_RPM = MAJOR_TICKS[MAJOR_TICK_COUNT-1];
const int HALF_CIRCLE_DEGREES = 180;
const float PI_RADIANS = PI/HALF_CIRCLE_DEGREES;

void drawTicks(const long*, int, int);
float getPercentMaxRpm(long);
float getCircleXWithLengthAndAngle(uint16_t, float);
float getCircleYWithLengthAndAngle(uint16_t, float);
void drawMajorTickLabels(void);
void drawIndicatorHand(long);
void drawRpm(long);
void drawCoolantTemp(int);
void displayInfo(const char*);


Adafruit_SSD1306 disp(4);
ObdReader elm({
  .rxPin = 8,
  .txPin = 9
});

void setup() {
  Serial.begin(9600);
  disp.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  disp.clearDisplay();
  disp.setCursor(0, 0);
  disp.setTextSize(TEXT_SIZE_LARGE);
  disp.setTextColor(WHITE);
  displayInfo("Setting up");
  elm.setup();
  displayInfo("Setup done");
  Serial.println("Setup done");
  delay(1000);
}

void loop() {
  delay(80);
  int rpm = elm.getRpm();
  if(rpm != 0) {
    // Serial.println("Get RPM");
    // Serial.println(rpm);
    disp.clearDisplay();
    drawRpm(rpm);
    drawCoolantTemp(110);
    disp.display();
  }
  // for(int i = 0;;i++) {
  //   delay(50);
  // }
}

void displayInfo(const char* text) {
  disp.clearDisplay();
  disp.setCursor(0, OLED_HEIGHT/2);
  disp.setTextSize(TEXT_SIZE_LARGE);
  disp.setTextColor(WHITE);
  disp.print(text);
  disp.display();
}

void drawRpm(long rpm) {
  disp.drawCircle(DIAL_CENTER_X, DIAL_CENTER_Y, DIAL_RADIUS, WHITE);
  drawTicks(MAJOR_TICKS, MAJOR_TICK_COUNT, MAJOR_TICK_LENGTH);
  drawTicks(MINOR_TICKS, MINOR_TICK_COUNT, MINOR_TICK_LENGTH);
  drawMajorTickLabels();
  drawIndicatorHand(rpm);
}

void drawTicks(const long ticks[], int tick_count, int tick_length) {
  for (int tick_index = 0; tick_index < tick_count; tick_index++) {
		long rpm_tick_value = ticks[tick_index];
		float tick_angle = (HALF_CIRCLE_DEGREES * getPercentMaxRpm(rpm_tick_value)) + HALF_CIRCLE_DEGREES;
		uint16_t dial_x = getCircleXWithLengthAndAngle(DIAL_RADIUS - 1, tick_angle);
		uint16_t dial_y = getCircleYWithLengthAndAngle(DIAL_RADIUS - 1, tick_angle);
		uint16_t tick_x = getCircleXWithLengthAndAngle(DIAL_RADIUS - tick_length, tick_angle);
		uint16_t tick_y = getCircleYWithLengthAndAngle(DIAL_RADIUS - tick_length, tick_angle);
		disp.drawLine(dial_x, dial_y, tick_x, tick_y, WHITE);
	}
}

float getPercentMaxRpm(long value) {
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
		long rpm_tick_value = MAJOR_TICKS[label_index];
		float tick_angle = (HALF_CIRCLE_DEGREES	* getPercentMaxRpm(rpm_tick_value)) + HALF_CIRCLE_DEGREES;
		uint16_t dial_x = getCircleXWithLengthAndAngle(LABEL_RADIUS, tick_angle);
		uint16_t dial_y = getCircleYWithLengthAndAngle(LABEL_RADIUS, tick_angle);
		disp.setCursor(dial_x - DIAL_LABEL_X_OFFSET, dial_y - DIAL_LABEL_Y_OFFSET);
		int label_value = rpm_tick_value / ONE_K;
		disp.print(label_value);
	}
}

void drawIndicatorHand(long rpm_value) {
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
  disp.print("TEMP: ");
  disp.setTextSize(TEXT_SIZE_SMALL);
  disp.print(temp);
  disp.drawCircle(86 + (temp > 100 ? 7 : 0), 2, 1, WHITE);
}
