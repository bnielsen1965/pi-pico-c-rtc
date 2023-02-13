
#include "ds3231.h"

// initialize ds3231 clock
int initDS3231 () {
  // initialize i2c hardware
  i2c_init(I2C_PORT, 100 * 1000);
  gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA_PIN);
  gpio_pull_up(I2C_SCL_PIN);
  return adjustDS3231Day();
}

// read DS3231 time into struct tm
int readDS3231Time (struct tm * datetime) {
  uint8_t reg = 0x00; // start register
  int status;
  uint8_t buffer[DS3231_TIME_REGISTER_COUNT];
  memset(buffer, 0, DS3231_TIME_REGISTER_COUNT);
  // write single byte to specify the starting register for follow up reads
  status = writeDS3231(&reg, 1);
  if (status) return status;
  // read all registers into buffer
  status = readDS3231(buffer, DS3231_TIME_REGISTER_COUNT);
  if (status) return status;
  // convert buffer to struct tm
  bufferToTime(buffer, datetime);
  return DS3231_NO_ERROR;
}

// set the DS3231 time to value in struct tm
int setDS3231Time (struct tm * datetime) {
  int status;
  uint8_t buffer[8];
  memset(buffer, 0, 8);
  timeToBuffer(datetime, buffer + 1);
  return writeDS3231(buffer, 8);
}

// adjust DS3231 day of week if value is incorrect
int adjustDS3231Day () {
  struct tm datetime;
  int status;
  // read current time from DS3231
  status = readDS3231Time(&datetime);
  if (status) return status;
  // note day of week from ds3231 before conversion process
  int day = datetime.tm_wday;
  // convert ds3231 time to Unix time value
  time_t t = mktime(&datetime);
  // convert Unix time value back to struct tm, this will set the proper tm_wday and tm_yday values
  struct tm * nt = gmtime(&t);
  // if tm_wday value does not match original day value then set ds3231 day value 
  if (day != nt->tm_wday) status = setDS3231DayTM(nt);
  return status;
}

// set the day of week DS3231 register from the struct tm tm_wday value
int setDS3231DayTM (struct tm * datetime) {
  return setDS3231Day((uint8_t)datetime->tm_wday + 1); // struct tm range is 0 to 6 while ds3231 uses 1 to 7
}

// set the day of week DS3231 register
int setDS3231Day (uint8_t day) {
  // preset buffer to write day of week at register
  uint8_t buffer[4] = {DS3231_REGISTER_DAY, day};
  int status = writeDS3231(buffer, 2);
  return status;
}


// write buffer to DS3231 registers
int writeDS3231 (uint8_t * buffer, int bufferLength) {
  int status = i2c_write_timeout_us(I2C_PORT, I2C_ADDR, buffer, bufferLength, true, DS3231_I2C_TIMEOUT);
  if (status == PICO_ERROR_GENERIC || status == PICO_ERROR_TIMEOUT) return statusToDS3231ErrorCode(status);
  return DS3231_NO_ERROR;
}

// read registers from DS3231 to buffer
int readDS3231 (uint8_t * buffer, int bufferLength) {
  int status = i2c_read_timeout_us(I2C_PORT, I2C_ADDR, buffer, bufferLength, true, DS3231_I2C_TIMEOUT);
  if (status == PICO_ERROR_GENERIC || status == PICO_ERROR_TIMEOUT) return statusToDS3231ErrorCode(status);
  return DS3231_NO_ERROR;
}


// convert DS3231 register buffer contents to struct tm
void bufferToTime (uint8_t * buffer, struct tm * datetime) {
  uint8_t tmp;
  //  buffer bcd time to seconds
  tmp = buffer[0];
  datetime->tm_sec = tmp & 0x0f;
  tmp >>= 4;
  datetime->tm_sec += 10 * (tmp & 0x7);
  // buffer bcd to minutes
  tmp = buffer[1];
  datetime->tm_min = tmp & 0x0f;
  tmp >>= 4;
  datetime->tm_min += 10 * (tmp & 0x7);
  // buffer bcd to hours
  tmp = buffer[2];
  // extract hours from the first nibble
  datetime->tm_hour = tmp & 0x0f;
  tmp >>= 4;
  // test if 12 hour mode
  if (tmp & 0x04) {
    // extract the 10 hour bit
    datetime->tm_hour += 10 * (tmp & 0x01);
    // test if PM and hour is not 12
    if (tmp & 0x02 && datetime->tm_hour != 12) datetime->tm_hour += 12;
    // else test if AM and hour is 12
    else if (!(tmp & 0x02) && datetime->tm_hour == 12) datetime->tm_hour = 0;
  }
  else {
    // extract the 10 and 20 hour bits in 24 hour mode
    datetime->tm_hour += 10 * (tmp & 0x03);
  }
  // day of week
  tmp = buffer[3];
  datetime->tm_wday = tmp & 0x07;
  datetime->tm_wday -= 1;
  // don't have day of year from clock
  datetime->tm_yday = -1;
  // don't have daylight savings time from clock
  datetime->tm_isdst = -1;
  // buffer bcd to day of month
  tmp = buffer[4];
  datetime->tm_mday = tmp & 0x0f;
  tmp >>= 4;
  datetime->tm_mday += 10 * (tmp & 0x03);
  // buffer bcd to month
  tmp = buffer[5];
  datetime->tm_mon = tmp & 0x0f;
  tmp >>= 4;
  datetime->tm_mon += 10 * (tmp & 0x01);
  datetime->tm_mon -= 1;
  // extract century
  datetime->tm_year = tmp & 0x08 ? 2000 : 1900;
  // buffer bcd to year
  tmp = buffer[6];
  datetime->tm_year += tmp & 0x0f;
  tmp >>= 4;
  datetime->tm_year += 10 * (tmp & 0x0f);
  datetime->tm_year -= 1900;
}

// convert struct tm to DS3231 register buffer
int timeToBuffer (struct tm * datetime, uint8_t * buffer) {
  uint8_t bcd_year = (((uint8_t)(datetime->tm_year % 100) / 10) << 4) | ((uint8_t)(datetime->tm_year % 100) % 10);
  uint8_t msb_century = (uint8_t)(datetime->tm_year / 100) << 7;
  uint8_t bcd_month = (((uint8_t)(datetime->tm_mon + 1) / 10) << 4) | (((uint8_t)(datetime->tm_mon + 1) % 10)) | msb_century;
  uint8_t bcd_date = (((uint8_t)(datetime->tm_mday) / 10) << 4) | (((uint8_t)(datetime->tm_mday) % 10));
  uint8_t bcd_dow = (((uint8_t)(datetime->tm_wday + 1) % 10));
  uint8_t bcd_hour = (((uint8_t)(datetime->tm_hour) / 10) << 4) | (((uint8_t)(datetime->tm_hour) % 10));
  uint8_t bcd_min = (((uint8_t)(datetime->tm_min) / 10) << 4) | (((uint8_t)(datetime->tm_min) % 10));
  uint8_t bcd_sec = (((uint8_t)(datetime->tm_sec) / 10) << 4) | (((uint8_t)(datetime->tm_sec) % 10));
  buffer[0] = bcd_sec;
  buffer[1] = bcd_min;
  buffer[2] = bcd_hour;
  buffer[3] = bcd_dow;
  buffer[4] = bcd_date;
  buffer[5] = bcd_month;
  buffer[6] = bcd_year;
  return DS3231_NO_ERROR;
}


// convert i2c IO status number to an error code
int statusToDS3231ErrorCode (int status) {
  if (status == PICO_ERROR_GENERIC) return DS3231_GENERIC_ERROR;
  else if (status == PICO_ERROR_TIMEOUT) return DS3231_TIMEOUT_ERROR;
  else return DS3231_UNKNOWN_ERROR;
}

// convert DS3231 error code to string
char * ds3231ErrorString (int status) {
  if (DS3231_NO_ERROR == status) return "No error.";
  if (DS3231_GENERIC_ERROR == status) return "Generic error.";
  if (DS3231_TIMEOUT_ERROR == status) return "Timeout error.";
  if (DS3231_UNKNOWN_ERROR == status) return "Unknown error.";
  return "Invalid status code.";
}
