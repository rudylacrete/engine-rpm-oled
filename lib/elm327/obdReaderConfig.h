#ifndef _OBD_READER_CONFIG_H
#define _OBD_READER_CONFIG_H

#include <inttypes.h>

typedef struct {
  unsigned int rxPin;
  unsigned int txPin;
} obd_reader_conf_t;

#endif