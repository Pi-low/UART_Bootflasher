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
    char pcDateTime[40];
    char pcFileName[40];
    time_t tsTime = time(NULL);
    struct tm *pTime = localtime(&tsTime);

    strftime(pcDateTime, 20, "%Y-%m-%d_%H.%M.%S", pTime);
    sprintf(pcFileName, "%s_log.txt", pcDateTime);

    LogStream = fopen(pcFileName, "w+");

    if (LogStream != NULL)
    {
        fprintf(LogStream, "<%s>: Bootloader session start\r\n", pcDateTime);
    }
}

void Logger_Append(char *FpcString)
{
    char pcDateTime[20];
    time_t tsTime = time(NULL);
    struct tm *pTime = localtime(&tsTime);
    strftime(pcDateTime, 20, "%H:%M:%S", pTime);

    if (LogStream != NULL && FpcString != NULL)
    {
        fprintf(LogStream, "<%s> %s", pcDateTime, FpcString);
    }
}

void Logger_AppendArray(char *FpcText, uint8_t *Fpu8Array, uint16_t Fu16Len)
{
    uint16_t u16Index = 0;
    char pcSmallString[20];

    if ((LogStream != NULL) && (Fpu8Array != NULL) && (FpcText!= NULL))
    {
        for (u16Index = 0; u16Index < Fu16Len; u16Index++)
        {
            if ((u16Index != 0) && ((u16Index % LOG_GROUP_BYTE_PER_LINE) == 0))
            {
                sprintf(pcSmallString, "\r\n");
                strcat(FpcText, pcSmallString);
            }
            sprintf(pcSmallString, "%02X ", Fpu8Array[u16Index]);
            strcat(FpcText, pcSmallString);
        }
        Logger_Append(FpcText);
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
