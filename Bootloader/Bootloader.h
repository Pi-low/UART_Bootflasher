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

#ifndef BOOTLOADER_H_INCLUDED
#define BOOTLOADER_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../Config.h"

#define MAX_BUFFER_SIZE     266
#define MAX_FRAME_LENGTH    262
#define BYTES_PER_BLOCK     256 /* which correspond to a flash row size in byte */
#define APPL_DESC_LEN       128
#define ADDR_START_BOOT     (0x400L << 1)
#define ADDR_START_APPLI    (0x4000L << 1)
#define ADDR_APPL_DESC      (0x280L << 1)
#define ADDR_APPL_VERSION   (0x300L << 1)
#define ADDR_APPL_END       (0x55400L << 1)

typedef struct
{
    uint32_t u32StartAddr;
    uint32_t u32EndAddr;
    uint16_t u16Len;
    uint8_t pu8Data[BYTES_PER_BLOCK];
    uint16_t u16CRCBlock;
} tsDataBlock;

typedef struct
{
    uint8_t u8ID;
    uint16_t u16Length;
    uint8_t pu8Payload[MAX_FRAME_LENGTH];
    uint8_t pu8Response[MAX_FRAME_LENGTH];
} tsFrame;

typedef enum
{
    eBoot =                     0x00,
    eService_gotoBoot =         0x01,
    eService_echo =             0x02,
    eService_getInfo =          0x03,
    eService_eraseFlash =       0x04,
    eService_dataTransfer =     0x05,
    eService_checkFlash =       0x06,
    eService_writePin =         0x0A,
    eService_readPin =          0x0B,
} teBootServices;

typedef enum
{
    eOperationSuccess = 0,
    eOperationNotAvailable,
    eOperationNotAllowed,
    eOperationFail,
    eBadChecksum,
    eBadFrameLength,
    eUnknownFrameID,
    eFrameTimeout,
    eFlashEraseError,
    eMemoryNotBlanked,
    eBadBlockAddr,
    eBadCRCBlock,
    eFlashWriteError,
    eAppliCheckError,
    eBootSessionTimeout,
    eBootIdle,
    eBootAttention,
    eBootSupportedReturnCode
} teOperationRetVal;

bool Bootloader_ProcessFile(FILE* FpHexFile, uint32_t Fu32FileSize);
bool Bootloader_GetInfoFromiHexFile(FILE* FpHexFile, uint32_t Fu32FileSize);
void Bootloader_PrintErrcode(uint8_t Fu8ErrCode);
bool Bootloader_RequestSwVersion(uint16_t* Fpu16Version);
bool Bootloader_RequestSwInfo(uint8_t* Fpu8Buf);
bool Bootloader_TransferData(tsDataBlock* FptsDataBlock);
bool Bootloader_RequestEraseFlash(void);
bool Bootloader_RequestBootSession(void);
bool Bootloader_CheckFlash(void);
void Bootloader_NotifyEndFlash(void);
void Bootloader_ManageCrcData(tsDataBlock* FptsDataBlock);
uint16_t Bootloader_GetCrcData(void);

#endif // BOOTLOADER_H_INCLUDED
