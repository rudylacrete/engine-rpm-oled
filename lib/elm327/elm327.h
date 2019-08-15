#ifndef OBD_READER_H
#define OBD_READER_H

#define BAUDRATE 38400
#define OBD_CMD_RETRIES 5
#define MAX_RESP_BUFFER 50

#include <inttypes.h>
#include <SoftwareSerial.h>
#include <obdReaderConfig.h>

typedef enum {
  NO_ERROR = 0,
  RESET_ERROR,
  ECHO_OFF_ERROR,
  GET_VOLTAGE_ERROR,
  OBD_NOT_CONNECTED,
  SELECT_PROTOCOL_ERROR,
  PID00_ERROR,
  PID20_ERROR,
  PID40_ERROR,
  UNKOWN_ERROR
} error_code_t;

class ObdReader{
  public:
    ObdReader(obd_reader_conf_t config): config(config), debug_mode(false) {};
    char* resBuf;
    uint8_t resLength;
    error_code_t setup();
    int getRpm();
    int getEngineCoolantTemp();
    void enable_debug(bool enabled);
  private:
    obd_reader_conf_t config;
    bool debug_mode;
    void debug(const char* message, bool new_line);
    void debug(const __FlashStringHelper* message, bool new_line);
    void printHex(const char* str, uint8_t size);
    SoftwareSerial *serial;
    char* send_OBD_cmd(const char* obd_cmd, bool waitPrompt);
    error_code_t obd_init();
    void replaceStrChar(char currentChr, char newChr);
};
#endif
