#ifndef OBD_READER_H
#define OBD_READER_H

#define BAUDRATE 38400
#define OBD_CMD_RETRIES 5
#define MAX_RESP_BUFFER 50

#include <inttypes.h>
#include <SoftwareSerial.h>
#include <obdReaderConfig.h>

typedef enum {
  AT = 1,
  COM
} mode_t;

class ObdReader{
  public:
    ObdReader(obd_reader_conf_t config): config(config), debug_mode(false) {};
    char* resBuf;
    uint8_t resLength;
    bool setup();
    int getRpm();
    int getEngineCoolantTemp();
    void enable_debug(bool enabled);
  private:
    bool debug_mode;
    void debug(const char* message, bool new_line = true);
    void debug(const __FlashStringHelper* message, bool new_line = true);
    void printHex(const char* str, uint8_t size);
    mode_t mode;
    SoftwareSerial *serial;
    obd_reader_conf_t config;
    char* send_OBD_cmd(const char* obd_cmd, bool waitPrompt = true);
    bool obd_init();
    void replaceStrChar(char currentChr, char newChr);
};
#endif
