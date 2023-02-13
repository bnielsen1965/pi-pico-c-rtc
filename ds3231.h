

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <time.h>
//#include "pico/util/datetime.h"

// DS3231 I2C settings
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define I2C_ADDR 0x68
#define DS3231_I2C_TIMEOUT 5000
// TODO do something different with buffer size
#define I2C_BUFFER_SIZE 0x12
#define DS3231_TIME_REGISTER_COUNT 0x12
#define DS3231_REGISTER_DAY 0x03

enum ds3231_error_codes {
  DS3231_NO_ERROR = 0,
  DS3231_GENERIC_ERROR,
  DS3231_TIMEOUT_ERROR,
  DS3231_UNKNOWN_ERROR
};

static const char * ds3231_errors[] = {
  [0] = "No error",
  [DS3231_GENERIC_ERROR] = "Generic error.",
  [DS3231_TIMEOUT_ERROR] = "Timeout error.",
  [DS3231_UNKNOWN_ERROR] = "Unknown error."
};


// initialize DS3231 rtc
int initDS3231 ();

// read and write DS3231 rtc registers
int readDS3231Time (struct tm * datetime);
int setDS3231Time (struct tm * datetime);
int adjustDS3231Day ();
int setDS3231DayTM (struct tm * datetime);
int setDS3231Day (uint8_t day);
int readDS3231 (uint8_t * buffer, int bufferLength);
int writeDS3231 (uint8_t * buffer, int bufferLength);

// DS3231 error handling
int statusToDS3231ErrorCode (int status);
char * ds3231ErrorString (int status);

// buffer timestamp conversion methods
int timeToBuffer (struct tm * datetime, uint8_t * buffer);
void bufferToTime (uint8_t * buffer, struct tm * datetime);
