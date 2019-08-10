#ifndef OBD_READER_H
#define OBD_READER_H

#define BAUDRATE 38400
#define OBD_CMD_RETRIES 5

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
    bool setup();
    int getRpm();
  private:
    mode_t mode;
    SoftwareSerial *serial;
    obd_reader_conf_t config;
    bool send_OBD_cmd(const char* obd_cmd);
    bool obd_init();
};
#endif
