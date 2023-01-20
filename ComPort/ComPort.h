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

#ifndef COMPORT_H_INCLUDED
#define COMPORT_H_INCLUDED

#define MAX_COM_PORT_NB (48)
#define UNIT_TIME_DIV (25) // time to transmit 266 bytes at 115200 bauds + extra
#define MAX_FRAME_FIFO_SIZE (16)

#include <stdint.h>
#include <stdbool.h>
#include "../Bootloader/Bootloader.h"

void ComPort_Scan(void);
bool ComPort_Open(char* FpcString);
void ComPort_Close(void);
bool ComPort_WaitForStartupSequence(uint16_t Fu16Timeout);
void ComPort_SendStandaloneFrame(tsFrame* FptsMsg);
bool ComPort_SendGenericFrame(tsFrame* FptsSendMsg, uint16_t Fu16Timeout);

#endif // COMPORT_H_INCLUDED
