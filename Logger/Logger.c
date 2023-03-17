/*
 *  The UART Flashing tool aims to flash embedded software for the
 *  "Big Brain" board system.
 *  Copyright (C) 2023 Nello CHOMMANIVONG
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../main.h"
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

    LogStream = fopen(pcFileName, "wb+");
    strftime(pcDateTime, 20, "%Y-%m-%d %H:%M:%S", pTime);
    if (LogStream != NULL)
    {
        fprintf(LogStream, "Tool build: %s\r\nRelease: %02u\r\n[%s]: Bootloader session start\r\n", pcBuildDateTime, u8ToolVersion, pcDateTime);
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
        fprintf(LogStream, "[%s] %s", pcDateTime, FpcString);
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
        sprintf(pcSmallString, "\r\n");
        strcat(FpcText, pcSmallString);
        Logger_Append(FpcText);
    }
}

void Logger_LineFeed(void)
{
    if (LogStream != NULL)
    {
        fprintf(LogStream, "\r\n");
    }
}

void Logger_Close(void)
{
    Logger_Append("Log Session end\r\n");
    if (LogStream != NULL)
    {
        fclose(LogStream);
    }
}
