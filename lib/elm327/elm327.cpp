/*
This source code is inspired from the following:
http://www.kokoras.com/OBD/Arduino_HC-05_ELM327_OBD_RPM_Shift_Light.htm
*/
#include <Arduino.h>
#include "elm327.h"

void printHex(const char*, uint8_t);

bool ObdReader::setup() {
  pinMode(config.rxPin, INPUT);
  pinMode(config.txPin, OUTPUT);

  serial = new SoftwareSerial(config.rxPin, config.txPin);
  serial->begin(BAUDRATE);

  resBuf = (char*) calloc(MAX_RESP_BUFFER, sizeof(char));
  resLength = 0;

  return obd_init();
}

void ObdReader::enable_debug(bool enabled) {
  debug_mode = enabled;
}

void ObdReader::debug(const char* message, bool new_line = true) {
  if(debug_mode) {
    new_line ? Serial.println(message) : Serial.print(message);
  }
}

void ObdReader::debug(const __FlashStringHelper* message, bool new_line = true) {
  if(debug_mode) {
    new_line ? Serial.println(message) : Serial.print(message);
  }
}

char* ObdReader::send_OBD_cmd(const char* obd_cmd, bool waitPrompt = true) {
  char recvChar;
  boolean prompt = false;
  int retries = 0;
  unsigned long time = millis();
  bool spinlock = true;

  while((!prompt) && (retries < OBD_CMD_RETRIES)) {                //while no prompt and not reached OBD cmd retries
    memset(resBuf, 0, MAX_RESP_BUFFER);
    resLength = 0;
    debug(F("Sending command "), false);
    debug(obd_cmd);
    serial->print(obd_cmd);                             //send OBD cmd
    serial->print("\r");                                //send cariage return

    while(spinlock){
      if(millis() > (time + 4000)){
        break;
      }
      else if(serial->available()>0){
        spinlock = false;
      }
    }
    // if serial port still not available
    // skip next instructions and make another cmd send attempt
    if(spinlock) {
      Serial.println(F("No bytes transferred. Send command again!"));
      retries++;                                          //increase retries
      delay(1000);
      continue;
    }

    while ((serial->available()>0) && (!prompt)) {       //while there is data and not prompt
      recvChar = serial->read();                        //read from elm
      if (recvChar == 62) {
        prompt = true;                            //if received char is '>' then prompt is true
      }
      // 13 correspond to '\r', 32 to ' '
      else if(recvChar != 13 && recvChar != 32) resBuf[resLength++] = recvChar;
    }
    // exit the loop if we are not waiting for prompt
    if(!waitPrompt) {
      break;
    }
    if(!prompt) {
      Serial.print(F("Get no prompt! Try again."));
      retries++;                                          //increase retries
      delay(1000);
    }
  }
  if(retries == OBD_CMD_RETRIES) {
    Serial.print(F("Reached max attempt. Abort!"));
    return NULL;
  } else {
    debug(F("Response: "), false);
    debug(resBuf);
    printHex(resBuf, resLength);
    return resBuf;
  }
}

void ObdReader::printHex(const char* str, uint8_t size) {
  if(debug_mode) {
    char hexStr[4] = {'\0'};
    for(int i = 0; i < size; i++) {
      sprintf(hexStr, "%x ", str[i]);
      Serial.print(hexStr);
    }
    Serial.println(F(" END"));
  }
}

void ObdReader::replaceStrChar(char currentChr, char newChr) {
  for(uint8_t i = 0; i < resLength; i++) {
    if(resBuf[i] == currentChr) {
      resBuf[i] = newChr;
    }
  }
}

bool ObdReader::obd_init() {
  float voltage = 0;

  if(send_OBD_cmd("ATZ") == NULL) return false;      //send to OBD ATZ, reset
  delay(1000);

  if(send_OBD_cmd("ATE0") == NULL) return false;      //send to OBD ATE0, echo off
  delay(5000);

  if(send_OBD_cmd("ATRV") == NULL) return false;      //read voltage to check if OBD is connected to car
  replaceStrChar('V', '\0');
  voltage = String(resBuf).toFloat();
  if(voltage < 6) {
    Serial.println(F("OBDII plug not connected"));
    return false;
  }
  delay(1000);

  send_OBD_cmd("ATSP0");    //send ATSP0, protocol auto
  delay(1000);

  send_OBD_cmd("0100");     //send 0100, retrieve available pid's 00-19
  delay(1000);

  send_OBD_cmd("0120");     //send 0120, retrieve available pid's 20-39
  delay(1000);

  send_OBD_cmd("0140");     //send 0140, retrieve available pid's 40-??
  delay(1000);

  return true;
}

int ObdReader::getRpm() {
  boolean valid = false;
  int rpm = 0;

  send_OBD_cmd("010C1", false);

  valid = ((resBuf[0] == '4') && (resBuf[1] == '1') && (resBuf[2] == '0') && (resBuf[3] == 'C')); //if first four chars after our command is 410C
  if (valid){                                                                    //in case of correct RPM response
    char hexByte[2];
    int A, B;
    //start calculation of real RPM value
    //RPM is coming from OBD in two 8bit(bytes) hex numbers for example A=0B and B=6C
    //the equation is ((A * 256) + B) / 4, so 0B=11 and 6C=108
    //so rpm=((11 * 256) + 108) / 4 = 731 a normal idle car engine rpm
    memcpy(hexByte, resBuf + 4, 2);
    A = strtol(hexByte, NULL, 16);
    memcpy(hexByte, resBuf + 6, 2);
    B = strtol(hexByte, NULL, 16);
    rpm = ((A * 256) + B) / 4;
  }
  return rpm;
}

int ObdReader::getEngineCoolantTemp() {
  boolean valid = false;
  int temp = 0;

  send_OBD_cmd("01051", false);

  valid = ((resBuf[0] == '4') && (resBuf[1] == '1') && (resBuf[2] == '0') && (resBuf[3] == '5'));
  if (valid){                                                                    //in case of correct RPM response
    char hexByte[2];
    int A;
    //start calculation of real RPM value
    //RPM is coming from OBD in two 8bit(bytes) hex numbers for example A=0B and B=6C
    //the equation is ((A * 256) + B) / 4, so 0B=11 and 6C=108
    //so rpm=((11 * 256) + 108) / 4 = 731 a normal idle car engine rpm
    memcpy(hexByte, resBuf + 4, 2);
    A = strtol(hexByte, NULL, 16);
    temp = A - 40;
  }
  return temp;
}
