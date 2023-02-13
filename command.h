
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "pico/mutex.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "console.h"
#include "ds3231.h"

// access to externally defined variables
extern semaphore_t semCommand;
extern char commandLine[];

void commandProcessor ();
void displayTime ();
void setZuluDateTime (char * str);
int checkISODelimiter (char * ptr, char delimiter, char * str);
int validateDateTime (struct tm * datetime);