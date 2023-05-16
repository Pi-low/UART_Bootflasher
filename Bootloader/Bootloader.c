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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../Core/Core.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "../ComPort/ComPort.h"
#include "../Crc16/Crc16.h"
#include "../Logger/Logger.h"
#include "../Config.h"
#include "Bootloader.h"

static uint32_t su32FileSize = 0;
static uint32_t su32TransferSizeCnt = 0;
static uint8_t spu8DataBuffer[ALLOCATION_SIZE + EXTENSION];
static uint16_t su16CRCFlash = 0;
static uint32_t su32CurFlashAddr = 0;
static uint16_t su16CRCBlockCnt = 0;
static uint8_t u8EndFlashFlag = 0;
static const char *spcErrCode[eBootSupportedReturnCode] =
    {
        "Operation successful",
        "Operation not available",
        "Operation not allowed",
        "Operation fail",
        "Bad frame checksum",
        "Bad frame length",
        "Unknown frame identifier",
        "Frame timeout",
        "Flash erase error",
        "Flash memory not blanked",
        "Incorrect block address",
        "CRC block error",
        "Flash write error",
        "Application check error",
        "Boot session timeout",
        "Entered boot idle mode",
        "Boot start",
};

bool Bootloader_ProcessFile(FILE *FpHexFile, uint32_t Fu32FileSize)
{
    bool RetVal = true;
    uint8_t *pu8LastData = NULL;
    uint32_t u32Read = 0, u32DataCnt = 0, u32Remain = 0;
    su32FileSize = Fu32FileSize;
    su32TransferSizeCnt = 0;

    ihex_set_callback_func((ihex_callback_fp)Core_CbDataBlockGen);
    ihex_reset_state();
    Core_InitDataBlockGen();
    Core_InitCRCBlockGen();
    u8EndFlashFlag = 0;
    fseek(FpHexFile, 0, SEEK_SET); /* Reset cursor */

    while ((u32DataCnt < Fu32FileSize) && (RetVal == true))
    {
        u32Read = fread(spu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE / BLOCK_SIZE), FpHexFile);
        if (u32Read != 0)
        {
            u32Read *= BLOCK_SIZE;
            u32DataCnt += u32Read;

            Core_PreParse(spu8DataBuffer, &u32Read);
            RetVal &= ihex_parser(spu8DataBuffer, u32Read);
#if PRINT_DEBUG_TRACE
            printf("[Read %u] parsed:%u\r\n", u32Read, RetVal);
#endif // PRINT_DEBUG_TRACE
        }
        else
        {
            fseek(FpHexFile, -(int)BLOCK_SIZE, SEEK_END); /* Set cursor to last block */
            u32Read = fread(spu8DataBuffer, BLOCK_SIZE, 1, FpHexFile);
            u32Remain = Fu32FileSize - u32DataCnt;
            if (u32Read != 0)
            {
                u32Read *= BLOCK_SIZE;
                u32DataCnt += u32Remain;

                pu8LastData = spu8DataBuffer;
                pu8LastData += u32Read - u32Remain;

                Core_PreParse(pu8LastData, &u32Remain);
                RetVal &= ihex_parser(pu8LastData, u32Remain);
#if PRINT_DEBUG_TRACE
                printf("[Read %u(%u)] parsed:%u\r\n", u32Read, u32Remain, RetVal);
#endif // PRINT_DEBUG_TRACE
                Core_CbDataBlockGen(0, NULL, 0);
                Bootloader_NotifyEndFlash();
            }
            else
            {
                printf("Read error, abort !\r\n");
                RetVal = false;
            }
        }
    }

    return RetVal;
}

bool Bootloader_GetInfoFromiHexFile(FILE *FpHexFile, uint32_t Fu32FileSize)
{
    bool RetVal = true;
    uint32_t u32Read = 0;

    ihex_reset_state();
    ihex_set_callback_func((ihex_callback_fp)Core_CbFetchLogisticData);
    fseek(FpHexFile, 0, SEEK_SET);

    do
    {
        u32Read = fread(spu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE / BLOCK_SIZE), FpHexFile);
        u32Read *= BLOCK_SIZE;
        Core_PreParse(spu8DataBuffer, &u32Read);
    } while (ihex_parser(spu8DataBuffer, u32Read));
    Core_GetSwInfo(NULL, NULL);
    return RetVal;
}

bool Bootloader_GetHexSizeBytes(FILE *FpHexFile, uint32_t Fu32FileSize)
{
    bool RetVal = true;
    uint32_t u32Read = 0;
    ihex_reset_state();
    ihex_set_callback_func((ihex_callback_fp)Core_CbGetEndAppAddress);
    do
    {
        /* code */
        u32Read = fread(spu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE / BLOCK_SIZE), FpHexFile);
        u32Read *= BLOCK_SIZE;
        Core_PreParse(spu8DataBuffer, &u32Read);
    } while (ihex_parser(spu8DataBuffer, u32Read));
    
}

void Bootloader_PrintErrcode(uint8_t Fu8ErrCode)
{
    char pcLogString[64];
    sprintf(pcLogString, "Target: %s\r\n", spcErrCode[Fu8ErrCode]);
    Logger_Append(pcLogString);
    printf("[Target]: %s\r\n", spcErrCode[Fu8ErrCode]);
}

bool Bootloader_RequestSwVersion(uint16_t *Fpu16Version)
{
    tsFrame tsSendMsg;
    char pcLogString[64];
    tsSendMsg.u8ID = eService_getInfo;
    tsSendMsg.u16Length = 1;
    tsSendMsg.pu8Payload[0] = 1;

#if PRINT_DEBUG_TRACE
    printf("[Info]: Request SW version\r\n");
#endif
    Logger_Append("Info: Request SW version\r\n");
#if SEND_UART
    if (ComPort_SendGenericFrame(&tsSendMsg, 1000) == true)
    {

        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            if (Fpu16Version != NULL)
            {
                *Fpu16Version = (uint16_t)tsSendMsg.pu8Response[1];
                *Fpu16Version <<= 8;
                *Fpu16Version |= (uint16_t)tsSendMsg.pu8Response[2];
            }
            if ((tsSendMsg.pu8Response[1] == 0xFF) && (tsSendMsg.pu8Response[2] == 0xFF))
            {
                printf("[Info]: no version, memory blank\r\n");
                Logger_Append("Info: no version, memory blank\r\n");
            }
            else
            {
                printf("[Target SW version]: %02X.%02X\r\n", tsSendMsg.pu8Response[1], tsSendMsg.pu8Response[2]);
                sprintf(pcLogString, "Target SW version: %02X.%02X\r\n", tsSendMsg.pu8Response[1], tsSendMsg.pu8Response[2]);
                Logger_Append(pcLogString);
            }
            return true;
        }
        else
        {
            Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
            return false;
        }
    }
    else
    {
        printf("[Error]: No response from target!\r\n");
        Logger_Append("Error: No response from target!\r\n");
        return false;
    }
#else
    return true;
#endif
}

bool Bootloader_RequestSwInfo(uint8_t *Fpu8Buf)
{
    tsFrame tsSendMsg;
    char pcLogString[512];
    tsSendMsg.u8ID = eService_getInfo;
    tsSendMsg.u16Length = 1;
    tsSendMsg.pu8Payload[0] = 2;

    uint8_t *pu8Intern = NULL;

#if PRINT_DEBUG_TRACE
    printf("Info: Request SW info\r\n");
#endif
    Logger_Append("Info: Request SW info\r\n");
#if SEND_UART
    if (Fpu8Buf != NULL)
    {
        pu8Intern = Fpu8Buf;
    }
    else
    {
        pu8Intern = malloc(128 * sizeof(uint8_t));
    }

    if (ComPort_SendGenericFrame(&tsSendMsg, 1000) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            if (tsSendMsg.pu8Response[1] == 0xFF)
            {
                printf("[Info]: no software info, memory blank\r\n");
                Logger_Append("Info: no software info, memory blank\r\n");
            }
            else
            {
                memcpy(pu8Intern, &tsSendMsg.pu8Response[1], 128);
                printf("[Target SW Info]: %s\r\n", pu8Intern);
                sprintf(pcLogString, "Target SW Info: %s\r\n", pu8Intern);
                Logger_Append(pcLogString);
            }
            return true;
        }
        else
        {
            Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
            return false;
        }
    }
    else
    {
        printf("[Error]: No response from target!\r\n");
        Logger_Append("Error: No response from target!\r\n");
        return false;
    }
#else
    return true;
#endif
}

bool Bootloader_TransferData(tsDataBlock *FptsDataBlock)
{
    bool bRetVal = true;
    char pcLogString[2048];
    tsFrame tsSendMsg;
    uint8_t *pu8FrameData = tsSendMsg.pu8Payload;
    uint16_t *pu16CRC = &FptsDataBlock->u16CRCBlock;
    uint16_t u16Cnt = 0, u16Tmp = 0;

    if (FptsDataBlock->u32StartAddr >= ADDR_APPL_END)
    {
        printf("[Info]: Address out of range, end flash\r\n");
        Logger_Append("Info: Address out of range, end flash\r\n");
        return false;
    }
    /* Load block length */
    tsSendMsg.u16Length = FptsDataBlock->u16Len + 5;
    tsSendMsg.u8ID = eService_dataTransfer;

    /* Load block address */
    *(pu8FrameData) = (FptsDataBlock->u32StartAddr >> 16) & 0x000000FF;
    *(pu8FrameData + 1) = (FptsDataBlock->u32StartAddr >> 8) & 0x000000FF;
    *(pu8FrameData + 2) = FptsDataBlock->u32StartAddr & 0x000000FF;

    Crc16_BufferUpdate(pu16CRC, pu8FrameData, 3);
    pu8FrameData += 3;

    for (u16Cnt = 0; u16Cnt < FptsDataBlock->u16Len; u16Cnt++)
    {
        *(pu8FrameData + u16Cnt) = FptsDataBlock->pu8Data[u16Cnt];
    }

    Crc16_BufferUpdate(pu16CRC, pu8FrameData, FptsDataBlock->u16Len);
    pu8FrameData += FptsDataBlock->u16Len;

    /* XOR CRC16 */
    u16Tmp = *(pu16CRC) ^ 0xFFFF;
    /* Reverse CRC16 MSB/LSB */
    *(pu8FrameData) = u16Tmp & 0x00FF;
    *(pu8FrameData + 1) = (u16Tmp >> 8) & 0x00FF;
    sprintf(pcLogString, "Generate block: address=0x%06X, length=%u(0x%03X), CRC frame=0x%02X%02X\r\n", FptsDataBlock->u32StartAddr,
            FptsDataBlock->u16Len,
            FptsDataBlock->u16Len,
            (uint8_t)u16Tmp,
            (uint8_t)(u16Tmp >> 8));
    su32TransferSizeCnt += FptsDataBlock->u16Len;
#if PRINT_DEBUG_TRACE && PRINT_BLOCK_RAW
    printf("\r\n[Block] address: 0x%06X length:%u\r\n", FptsDataBlock->u32StartAddr, FptsDataBlock->u16Len);
    for (u16Cnt = 0; u16Cnt < tsSendMsg.u16Length; u16Cnt++)
    {
        if (((u16Cnt % PRINT_GROUP_BYTE_PER_LINE) == 0) && (u16Cnt != 0))
        {
            printf("\r\n");
        }
        if ((u16Cnt == 0) || (u16Cnt == (tsSendMsg.u16Length - 2)))
        {
            printf("[");
        }
        printf("%02X", tsSendMsg.pu8Payload[u16Cnt]);
        if ((u16Cnt == 2) || (u16Cnt == (tsSendMsg.u16Length - 1)))
        {
            printf("]");
        }
        printf(" ");
    }
    printf("\r\n");
#else
    printf("[Sending]: @ 0x%06X : %u\%", FptsDataBlock->u32StartAddr, (su32TransferSizeCnt * 100) / su32FileSize);
#endif
#if SEND_UART
    if (ComPort_SendGenericFrame(&tsSendMsg, 4000) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            bRetVal = true;
        }
        else
        {
            bRetVal = false;
            Logger_LineFeed();
            Logger_AppendArray(pcLogString, FptsDataBlock->pu8Data, FptsDataBlock->u16Len);
        }
#if !PRINT_DEBUG_TRACE
        printf(" : %s\r\n", spcErrCode[tsSendMsg.pu8Response[0]]);
        sprintf(pcLogString, "Target : %s\r\n", spcErrCode[tsSendMsg.pu8Response[0]]);
        Logger_Append(pcLogString);
#else
        printf("\r\n");
        Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
#endif
    }
    else
    {
        bRetVal = false;
#if !PRINT_DEBUG_TRACE
        printf(" : Timeout/no response !\r\n");
#else
        printf("[Error] : Timeout/no response !\r\n");
#endif
        Logger_Append("Error: Timeout/no response !\r\n");
    }
#endif
    return bRetVal;
}

bool Bootloader_RequestEraseFlash(void)
{
    tsFrame tsSendMsg;
    tsSendMsg.u8ID = eService_eraseFlash;
    tsSendMsg.u16Length = 0;

    printf("[Info]: Request flash erase...\r\n");
    Logger_Append("Info: Request flash erase...\r\n");
#if SEND_UART
    if (ComPort_SendGenericFrame(&tsSendMsg, 10000) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            printf("[Target]: Flash memory erased!\r\n");
            Logger_Append("Target: Flash memory erased!\r\n");
            su16CRCFlash = 0;
            su32CurFlashAddr = 0;
            su16CRCBlockCnt = 0;
            u8EndFlashFlag = 0;
            return true;
        }
        else
        {
            Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
            return false;
        }
    }
    else
    {
        printf("[Error]: No response from target!\r\n");
        Logger_Append("Error: No response from target!\r\n");
        return false;
    }
#else
    return true;
#endif
}

bool Bootloader_RequestBootSession(void)
{
    tsFrame tsSendMsg;
    tsSendMsg.u8ID = eService_gotoBoot;
    tsSendMsg.u16Length = 0;

    printf("[Info]: Request boot session\r\n");
    Logger_Append("Info: Request boot session\r\n");
#if SEND_UART
    if (ComPort_SendGenericFrame(&tsSendMsg, 1000) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            printf("[Target]: Boot session started!\r\n");
            Logger_Append("Target: Boot session started!\r\n");
            return true;
        }
        else
        {
            Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
            return false;
        }
    }
    else
    {
        printf("[Error]: No response from target!\r\n");
        Logger_Append("Error: No response from target!\r\n");
        return false;
    }
#else
    return true;
#endif
}

bool Bootloader_CheckFlash(void)
{
    tsFrame tsSendMsg;
    char pcLogString[128];
    uint16_t u16CRC = su16CRCFlash ^ 0xFFFF;
    if (u8EndFlashFlag != 0)
    {
        tsSendMsg.u8ID = eService_checkFlash;
        tsSendMsg.u16Length = 4;
        tsSendMsg.pu8Payload[0] = u16CRC & 0xFF;
        tsSendMsg.pu8Payload[1] = (u16CRC >> 8) & 0xFF;
        tsSendMsg.pu8Payload[2] = (su16CRCBlockCnt >> 8) & 0x00FF;
        tsSendMsg.pu8Payload[3] = su16CRCBlockCnt & 0x00FF;
        printf("[Info]: Request flash check (0x%04X -> 0x%02X%02X)\r\n", su16CRCFlash,
               tsSendMsg.pu8Payload[0],
               tsSendMsg.pu8Payload[1]);
        sprintf(pcLogString, "Info: Request flash check 0x%04X:0x%02X%02X\r\n", su16CRCFlash,
                tsSendMsg.pu8Payload[0],
                tsSendMsg.pu8Payload[1]);
        Logger_Append(pcLogString);
#if SEND_UART
        if (ComPort_SendGenericFrame(&tsSendMsg, 5000) == true)
        {
            if (tsSendMsg.pu8Response[0] == eOperationSuccess)
            {
                printf("[Target]: Flash memory checked!\r\n");
                Logger_Append("Target: Flash memory checked!\r\n");
                return true;
            }
            else
            {
                Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
                printf("[Target] : IVT=0x%02X%02X APP+IVT=0x%02X%02X CHECK=0x%02X%02X\r\n", tsSendMsg.pu8Response[1],
                       tsSendMsg.pu8Response[2],
                       tsSendMsg.pu8Response[3],
                       tsSendMsg.pu8Response[4],
                       tsSendMsg.pu8Response[5],
                       tsSendMsg.pu8Response[6]);

                sprintf(pcLogString, "Target: IVT=0x%02X%02X APP+IVT=0x%02X%02X CHECK=0x%02X%02X\r\n", tsSendMsg.pu8Response[1],
                        tsSendMsg.pu8Response[2],
                        tsSendMsg.pu8Response[3],
                        tsSendMsg.pu8Response[4],
                        tsSendMsg.pu8Response[5],
                        tsSendMsg.pu8Response[6]);
                Logger_Append(pcLogString);
                return false;
            }
        }
        else
        {
            printf("[Error]: No response from target!\r\n");
            Logger_Append("Error: No response from target!\r\n");
            return false;
        }
#else
        return true;
#endif
    }
    else
    {
        return false;
    }
}

void Bootloader_NotifyEndFlash(void)
{
    u8EndFlashFlag = 1;
}

void Bootloader_ManageCrcData(tsDataBlock *FptsDataBlock)
{
    char pcLogString[128];
#if PRINT_DEBUG_TRACE && PRINT_BLOCK_CRC
    uint16_t u16Cnt;
    printf("\r\n[CRC]: Block %u: 0x%06X (%u)\r\n", su16CRCBlockCnt, FptsDataBlock->u32StartAddr, FptsDataBlock->u16Len);
    for (u16Cnt = 0; u16Cnt < FptsDataBlock->u16Len; u16Cnt++)
    {
        if (((u16Cnt % PRINT_GROUP_BYTE_PER_LINE) == 0) && (u16Cnt != 0))
        {
            printf("\r\n");
        }
        printf("%02X ", FptsDataBlock->pu8Data[u16Cnt]);
    }
    printf("\r\n");
#endif
    Crc16_BufferUpdate(&su16CRCFlash, FptsDataBlock->pu8Data, FptsDataBlock->u16Len);
    sprintf(pcLogString, "Crc Block %u, start address=0x%06X, CRC=0x%04X\r", su16CRCBlockCnt, FptsDataBlock->u32StartAddr, su16CRCFlash);
    Logger_Append(pcLogString);
    su16CRCBlockCnt++;
}

uint16_t Bootloader_GetCrcData(void)
{
    return su16CRCFlash;
}