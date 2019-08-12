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
    ObdReader(obd_reader_conf_t config): config(config) {};
    char* resBuf;
    uint8_t resLength;
    bool setup();
    int getRpm();
  private:
    mode_t mode;
    SoftwareSerial *serial;
    obd_reader_conf_t config;
    char* send_OBD_cmd(const char* obd_cmd);
    bool obd_init();
    void replaceStrChar(char currentChr, char newChr);
};
#endif
