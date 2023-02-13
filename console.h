

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/mutex.h"

// console line settings
#define LINE_LEN 80

// declare console methods
void console ();
void initConsole (char * lineBuffer, char * commandLine);
size_t appendChar(char * buffer, char c);
size_t deleteChar(char * buffer);
size_t clearBuffer (char * buffer);
void flushInput ();

// console needs access to external command semaphore
extern semaphore_t semCommand;
