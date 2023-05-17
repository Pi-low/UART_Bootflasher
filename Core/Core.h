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

#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#include <stdint.h>

void Core_InitDataBlockGen(void);
void Core_InitCRCBlockGen(void);
bool Core_CbDataBlockGen(uint32_t Fu32addr, const uint8_t *Fpu8Buffer, uint8_t Fu8Size);
bool Core_CbFetchLogisticData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
bool Core_CbGetEndAppAddress(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
void Core_PreParse(uint8_t *Fpu8Buffer, uint32_t *Fpu32Len);
void Core_GetSwInfo(uint16_t* Fpu16Version, uint8_t* Fpu8Decription);

extern uint32_t gu32HexByteCount;

#endif // CORE_H_INCLUDED
