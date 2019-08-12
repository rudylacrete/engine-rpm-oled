/*
This source code is inspired from the following:
http://www.kokoras.com/OBD/Arduino_HC-05_ELM327_OBD_RPM_Shift_Light.htm
*/
#include <Arduino.h>
#include "elm327.h"

void printHex(const char*, uint8_t);

bool ObdReader::setup() {
  // pinMode(config.rxPin, INPUT);
  // pinMode(config.txPin, OUTPUT);

  serial = new SoftwareSerial(config.rxPin, config.txPin);
  serial->begin(BAUDRATE);

  return obd_init();
}

char* ObdReader::send_OBD_cmd(const char* obd_cmd) {
  char result[MAX_RESP_BUFFER];
  char recvChar;
  boolean prompt = false;
  int retries = 0;
  unsigned long time = millis();
  bool spinlock = true;
  uint8_t respSize;

  while((!prompt) && (retries < OBD_CMD_RETRIES)) {                //while no prompt and not reached OBD cmd retries
    memset(result, 0, MAX_RESP_BUFFER);
    respSize = 0;
    Serial.print("Sending command ");
    Serial.println(obd_cmd);
    serial->print(obd_cmd);                             //send OBD cmd
    serial->print("\r");                                //send cariage return

    while(spinlock){
      if(millis() > (time + 4000)){
        break;
      }
      else if(serial->available()){
        spinlock = false;
      }
    }
    // if serial port still not available
    // skip next instructions and make another cmd send attempt
    if(spinlock) {
      Serial.print("Port is not available. Send command again!");
      retries++;                                          //increase retries
      delay(1000);
      continue;
    }

    Serial.println("Spinlock ok");
    while (serial->available() && (!prompt)) {       //while there is data and not prompt
      recvChar = serial->read();                        //read from elm
      if (recvChar == 62) {
        prompt = true;                            //if received char is '>' then prompt is true
      }
      // 13 correspond to '\r'
      else if(recvChar != 13) result[respSize++] = recvChar;
    }
    if(!prompt) {
      Serial.print("Get no prompt! Try again.");
      retries++;                                          //increase retries
      delay(1000);
    }
    Serial.print("Response: ");
    Serial.println(result);

  }
  if(retries == OBD_CMD_RETRIES) {
    Serial.print("Reached max attempt. Abort!");
    return NULL;
  } else {
    Serial.print("Response: ");
    Serial.println(result);
    printHex(result, respSize);
    return result;
  }
}

void printHex(const char* str, uint8_t size) {
  char hexStr[4] = {'\0'};
  for(int i = 0; i < size; i++) {
    sprintf(hexStr, "%x ", str[i]);
    Serial.print(hexStr);
  }
  Serial.println(" END");
}

bool ObdReader::obd_init() {
  String res = "";

  if(send_OBD_cmd("ATZ") == NULL) return false;      //send to OBD ATZ, reset
  delay(100);

  if(send_OBD_cmd("ATE0") == NULL) return false;      //send to OBD ATE0, echo off
  delay(5000);

  if((res = send_OBD_cmd("ATRV")) == NULL) return false;      //read voltage to check if OBD is connected to car
  delay(1000);
  return true;

  res = send_OBD_cmd("ATSP0");    //send ATSP0, protocol auto
  if(!res) return res;
  delay(1000);

  res = send_OBD_cmd("0100");     //send 0100, retrieve available pid's 00-19
  if(!res) return res;
  delay(1000);

  res = send_OBD_cmd("0120");     //send 0120, retrieve available pid's 20-39
  if(!res) return res;
  delay(1000);

  res = send_OBD_cmd("0140");     //send 0140, retrieve available pid's 40-??
  if(!res) return res;
  delay(1000);

  return res;
}

int ObdReader::getRpm() {
  boolean prompt = false, valid = false;
  char recvChar;
  char bufin[15] = { '\0' };
  int i = 0, rpm = 0;

  serial->print("010C1");                        //send to OBD PID command 010C is for RPM, the last 1 is for ELM to wait just for 1 respond (see ELM datasheet)
  serial->print("\r");                           //send to OBD cariage return char
  while (serial->available() <= 0);              //wait while no data from ELM

  while ((serial->available()>0) && (!prompt)){  //if there is data from ELM and prompt is false
    recvChar = serial->read();                   //read from ELM
    if ((i < 15) && (!(recvChar == 32 || recvChar == 62 || recvChar == '\r'))) {                     //the normal respond to previus command is 010C1/r41 0C ?? ??>, so count 15 chars and ignore char 32 which is space
      bufin[i] = recvChar;                                 //put received char in bufin array
      i++;                                             //increase i
    }
    if (recvChar == 62) prompt = true;                       //if received char is 62 which is '>' then prompt is true, which means that ELM response is finished
  }

  valid = ((bufin[0] == '4') && (bufin[1] == '1') && (bufin[2] == '0') && (bufin[3] == 'C')); //if first four chars after our command is 410C
  if (valid){                                                                    //in case of correct RPM response
    char hexByte[2];
    int A, B;
    //start calculation of real RPM value
    //RPM is coming from OBD in two 8bit(bytes) hex numbers for example A=0B and B=6C
    //the equation is ((A * 256) + B) / 4, so 0B=11 and 6C=108
    //so rpm=((11 * 256) + 108) / 4 = 731 a normal idle car engine rpm
    memcpy(hexByte, bufin + 4, 2);
    A = strtol(hexByte, NULL, 16);
    memcpy(hexByte, bufin + 6, 2);
    B = strtol(hexByte, NULL, 16);
    rpm = ((A * 256) + B) / 4;
  }
  return rpm;
}
