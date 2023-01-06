#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "Logger.h"

FILE * LogStream = NULL;

void Logger_Init(void)
{
    LogStream = fopen("log.txt", "w+");
    if (LogStream != NULL)
    {
        fprintf(LogStream, "%s %s\r\nLog Session start\r\n", __DATE__, __TIME__);
    }
}

void Logger_Log(char* FpcString)
{
    if (LogStream != NULL)
    {
        fprintf(LogStream, "%s\r\n", FpcString);
    }
}

void Logger_Close(void)
{
    if (LogStream != NULL)
    {
        fprintf(LogStream, "Log Session end\r\n");
        fclose(LogStream);
    }
}
