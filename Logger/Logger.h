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

#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#define LOG_GROUP_BYTE_PER_LINE (32)

extern FILE * LogStream;

void Logger_Init(void);
void Logger_Append(char* FpcString);
void Logger_Close(void);
void Logger_AppendArray(char *FpcText, uint8_t *Fpu8Array, uint16_t Fu16Len);
void Logger_LineFeed(void);

#endif // LOGGER_H_INCLUDED
