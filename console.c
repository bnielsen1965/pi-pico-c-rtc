
#include "console.h"

// define command line buffer that will be used internallly and externally
char commandLine[LINE_LEN + 1];

// console process thread to handle stdin
void console () {
  int16_t c; // must be 16 bit to get the error codes
  char lineBuffer[LINE_LEN + 1]; // define input buffer
  size_t cursor = 0;
  // initialize console
  initConsole (lineBuffer, commandLine);

  // main console loop
  for (;;) {
    c = getchar();
    // some characters will not be processed
    if (c <= 0x07) continue;
    if (c >= 0x09 && c <= 0x0c) continue;
    if (c >= 0x0e && c <= 0x1a) continue;
    if (c >= 0x1c && c <= 0x1f) continue;
    // handle escape sequence
    if (c == 0x1b) {
      flushInput();
      continue;
    }
    // handle enter
    if (c == 0x0d) {
      // copy input line buffer to command line buffer
      memcpy(commandLine, lineBuffer, LINE_LEN);
      // clear input line buffer and send newline to terminal
      cursor = clearBuffer(lineBuffer);
      printf("\n");
      // release command semaphore to enable processing of command line
      sem_release(&semCommand);
      continue;
    }
    // handle backspace / del
    if (c == 0x7f || c == 0x08) {
      if (cursor == 0) continue;
      cursor = deleteChar(lineBuffer);
      // use backspace, space, backspace to erase character on terminal
      printf("%c%c%c", 0x08, ' ', 0x08);
      continue;
    }
    // don't allow characters when at end of buffer
    if (cursor == LINE_LEN) continue;
    // handle character input
    cursor = appendChar(lineBuffer, c);
    printf("%c", c);
  }
}


// initialize console
void initConsole (char * lineBuffer, char * commandLine) {
  // clear line buffers and set end of line nulls
  clearBuffer(commandLine);
  commandLine[LINE_LEN] = '\0';
  clearBuffer(lineBuffer);
  lineBuffer[LINE_LEN] = '\0';
  // flush all input
  flushInput();
}

// clear contents of the specified buffer
size_t clearBuffer (char * buffer) {
  memset(buffer, '\0', LINE_LEN);
  return strlen(buffer);
}

// append a character to the input buffer and return new length
size_t appendChar (char * buffer, char c) {
  if (strlen(buffer) == LINE_LEN) return LINE_LEN;
  buffer[strlen(buffer)] = c;
  return strlen(buffer);
}

// delete character from input buffer and return length
size_t deleteChar (char * buffer) {
  if (strlen(buffer) == 0) return 0;
  buffer[strlen(buffer) - 1] = '\0';
  return strlen(buffer);
}

// flush stdio input
void flushInput () {
  int16_t c; // must be 16 bit to capture error codes
  do {
    // read characters until nothing left to read
    c = getchar_timeout_us(500);
  } while (c != PICO_ERROR_TIMEOUT && c != PICO_ERROR_GENERIC);
}