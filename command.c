
#include "command.h"
#include "pico/bootrom.h"
#include <time.h>


// help message
const char * HELP = 
  "Pi Pico Basic Command Prompt - A simple 80 character command\n"
  "line buffer used to control the Pi Pico\n"
  "Commands:\n"
  "help - Display this help message\n"
  "bootrom - Reset and boot into BOOTSEL ROM\n"
  "time - Display current rtc timestamp\n"
  "setzulu - Set rtc using ISO zulu timestamp, I.E. 2023-02-12T13:22:18Z\n";


// thread to process commands
void commandProcessor () {
  printf("Initialize DS3231...\n");
  int status = initDS3231();
  if (status) printf("Error occurred during DS3231 initialization. %s\n", ds3231ErrorString(status));
  else printf("DS3231 initialized.\n");

  for (;;) {
    clearBuffer(commandLine);
    // block until console permits command processing
    sem_acquire_blocking(&semCommand);
    // continue if command line is empty
    if (0 == strlen(commandLine)) continue;
    // help command
    else if (0 == strcmp(commandLine, "help")) printf("%s", HELP);
    // bootrom command
    else if (0 == strcmp(commandLine, "bootrom")) reset_usb_boot(0, 0);
    // show us time since start
    else if (0 == strcmp(commandLine, "time")) displayTime();
    // set date time from ISO string
    else if (0 == strncmp(commandLine, "setzulu ", 8) && strlen(commandLine) > 8) setZuluDateTime(commandLine + 8);


    else if (0 == strncmp(commandLine, "dow ", 4) && strlen(commandLine) > 4) {
      char * ptr;
      long dow = strtol(commandLine + 4, &ptr, 10);
      if (dow < 1 || dow > 7) {
        printf("Invalid day value, must be 1 to 7.\n");
        continue;
      }
      int status = setDS3231Day(dow);
      printf("Status: %s\n", ds3231ErrorString(status));
    }


    // invalid input error
    else printf("Unknown command %s. Enter help command for help.\n", commandLine);
  }
}

// display current DS3231 time
void displayTime () {
  struct tm datetime;
  int status = readDS3231Time(&datetime);
  if (status) {
    printf("Error reading time, %s\n", ds3231ErrorString(status));
    return;
  }
  printf("%s", asctime(&datetime));
}

// set DS3231 time from ISO formatted zulu time string
void setZuluDateTime (char * str) {
  struct tm datetime;
  char * startPtr = str;
  char * endPtr;
  long year = strtol(startPtr, &endPtr, 10);
  if (0 != checkISODelimiter(endPtr, '-', str)) return;
  startPtr = endPtr;
  long month = strtol(startPtr + 1, &endPtr, 10);
  if (0 != checkISODelimiter(endPtr, '-', str)) return;
  startPtr = endPtr;
  long day = strtol(startPtr + 1, &endPtr, 10);
  if (0 != checkISODelimiter(endPtr, 'T', str)) return;
  startPtr = endPtr;
  long hour = strtol(startPtr + 1, &endPtr, 10);
  if (0 != checkISODelimiter(endPtr, ':', str)) return;
  startPtr = endPtr;
  long min = strtol(startPtr + 1, &endPtr, 10);
  if (0 != checkISODelimiter(endPtr, ':', str)) return;
  startPtr = endPtr;
  double sec = strtod(startPtr + 1, &endPtr);
  // if time zone specified make sure it is zulu
  if (endPtr[0] && endPtr[0] != 'Z' && endPtr[0] != 'z') {
    printf("Invalid time zone, must be Zulu.\n");
    return;
  }
  datetime.tm_year = year - 1900;
  datetime.tm_mon = month - 1;
  datetime.tm_mday = day;
  datetime.tm_hour = hour;
  datetime.tm_min = min;
  datetime.tm_sec = sec;
  if (0 != validateDateTime(&datetime)) return;
  // use time conversions to set tm_wday and tm_yday to correct values
  time_t t = mktime(&datetime);
  struct tm * nt = gmtime(&t);
  // set time on rtc
  int status = setDS3231Time (nt);
  if (status) printf("Error setting time, %s\n", ds3231ErrorString(status));
}

// check for expected delimiter character in string
int checkISODelimiter (char * ptr, char delimiter, char * str) {
  if (ptr[0] != delimiter) {
    printf("Invalid ISO format %s\n", str);
    return -1;
  }
  return 0;
}

// validate time values are valid
int validateDateTime (struct tm * datetime) {
  int status = 0;
  if (datetime->tm_sec < 0 || datetime->tm_sec > 61) {
    printf("Invalid seconds, must be from 0 to 61.\n");
    status = -1;
  }
  if (datetime->tm_min < 0 || datetime->tm_min > 59) {
    printf("Invalid minutes, must be from 0 to 59.\n");
    status = -1;
  }
  if (datetime->tm_hour < 0 || datetime->tm_hour > 23) {
    printf("Invalid hour, must be from 0 to 23.\n");
    status = -1;
  }
  if (datetime->tm_mday < 1 || datetime->tm_mday > 31) {
    printf("Invalid day of month, must be from 1 to 31.\n");
    status = -1;
  }
  if (datetime->tm_mon < 0 || datetime->tm_mon > 11) {
    printf("Invalid month, must be from 1 to 12.\n");
    status = -1;
  }
  if (datetime->tm_year < 0 || datetime->tm_year > 199) {
    printf("Invalid year, must be from 1900 to 2099.\n");
    status = -1;
  }
  return status;
}
