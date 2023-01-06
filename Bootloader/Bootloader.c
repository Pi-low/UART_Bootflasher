#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../Core/Core.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "../ComPort/ComPort.h"
#include "../Crc16/Crc16.h"
#include "Bootloader.h"

static uint8_t spu8DataBuffer[ALLOCATION_SIZE + EXTENSION];
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
    "Boot session timeout"
};

bool Bootloader_ProcessFile(FILE * FpHexFile, uint32_t Fu32FileSize)
{
    bool RetVal = true;
    uint8_t* pu8LastData = NULL;
    uint32_t u32Read = 0, u32DataCnt = 0, u32Remain = 0;

    ihex_set_callback_func((ihex_callback_fp)Core_CbDataBlockGen);
    ihex_reset_state();
    Core_InitDataBlockGen();
    fseek(FpHexFile, 0, SEEK_SET); /* Reset cursor */

    while ((u32DataCnt < Fu32FileSize) && (RetVal == true))
    {
        u32Read = fread(spu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE/BLOCK_SIZE), FpHexFile);
        if (u32Read != 0)
        {
            u32Read *= BLOCK_SIZE;
            u32DataCnt += u32Read;

            Core_PreParse(spu8DataBuffer, &u32Read);
#if PRINT_DEBUG_TRACE == 1
            printf("Read %u:\r\n", u32Read);
#endif // PRINT_DEBUG_TRACE
            RetVal &= ihex_parser(spu8DataBuffer, u32Read);
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
#if PRINT_DEBUG_TRACE == 1
                printf("Read %u(%u): ", u32Read, u32Remain);
#endif // PRINT_DEBUG_TRACE
                RetVal &= ihex_parser(pu8LastData, u32Remain);
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

bool Bootloader_GetInfoFromiHexFile(FILE* FpHexFile, uint32_t Fu32FileSize)
{
    bool RetVal = true;
    uint32_t u32Read = 0;

    ihex_reset_state();
    ihex_set_callback_func((ihex_callback_fp)Core_CbFetchLogisticData);
    fseek(FpHexFile, 0, SEEK_SET);

    do
    {
        u32Read = fread(spu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE/BLOCK_SIZE), FpHexFile);
        u32Read *= BLOCK_SIZE;
        Core_PreParse(spu8DataBuffer, &u32Read);
    } while (ihex_parser(spu8DataBuffer, u32Read));
    Core_GetSwInfo(NULL, NULL);
    return RetVal;
}

void Bootloader_PrintErrcode(uint8_t Fu8ErrCode)
{
    printf("[Target]: %s\r\n", spcErrCode[Fu8ErrCode]);
}

bool Bootloader_RequestSwVersion(uint16_t* Fpu16Version)
{
    tsFrame tsSendMsg;

    tsSendMsg.u8ID = eService_getInfo;
    tsSendMsg.u16Length = 1;
    tsSendMsg.pu8Payload[0] = 1;

    if (ComPort_SendGenericFrame(&tsSendMsg, 50) == true)
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
            }
            else
            {
                printf("[Target SW version]: %02X.%02X\r\n", tsSendMsg.pu8Response[1], tsSendMsg.pu8Response[2]);
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
        return false;
    }
}

bool Bootloader_RequestSwInfo(uint8_t* Fpu8Buf)
{
    tsFrame tsSendMsg;

    tsSendMsg.u8ID = eService_getInfo;
    tsSendMsg.u16Length = 1;
    tsSendMsg.pu8Payload[0] = 2;

    uint8_t* pu8Intern = NULL;

    if (Fpu8Buf != NULL)
    {
        pu8Intern = Fpu8Buf;
    }
    else
    {
        pu8Intern = malloc(64 * sizeof(uint8_t));
    }

    if (ComPort_SendGenericFrame(&tsSendMsg, 100) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            if (tsSendMsg.pu8Response[1] == 0xFF)
            {
                printf("[Info]: no software info, memory blank\r\n");
            }
            else
            {
                memcpy(pu8Intern, &tsSendMsg.pu8Response[1], 128);
                printf("[Target SW Info]: %s\r\n", pu8Intern);
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
        return false;
    }
}

bool Bootloader_TransferData(tsDataBlock* FptsDataBlock)
{
    bool bRetVal = true;
    tsFrame tsSendMsg;
    uint8_t* pu8FrameData = tsSendMsg.pu8Payload;
    uint16_t* pu16CRC = &FptsDataBlock->u16CRCBlock;
    uint16_t u16Cnt = 0, u16Tmp = 0;

    tsSendMsg.u16Length = FptsDataBlock->u16Len + 5;
    tsSendMsg.u8ID = eService_dataTransfer;

    /* Load block address */
   *(pu8FrameData) =     (FptsDataBlock->u32StartAddr >> 16) & 0x000000FF;
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

#if PRINT_DEBUG_TRACE == 1
    for (u16Cnt = 0; u16Cnt < tsSendMsg.u16Length; u16Cnt++)
    {
        if ((u16Cnt % 32) == 0)
        {
            printf("\r\n ");
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
    printf("[Sending block]: 0x%06X : %u (0x%02X%02X)\r\n", FptsDataBlock->u32StartAddr, FptsDataBlock->u16Len, (uint8_t)u16Tmp, (uint8_t)(u16Tmp >> 8));
#endif

#if PRINT_DEBUG_TRACE == 0
    if (ComPort_SendGenericFrame(&tsSendMsg, 100) == true)
#else
    if (ComPort_SendGenericFrame(&tsSendMsg, 25) == true)
#endif
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {

            bRetVal = true;
        }
        else
        {
            bRetVal = false;
        }
        Bootloader_PrintErrcode(tsSendMsg.pu8Response[0]);
    }
    else
    {
        bRetVal = false;
        printf("[Error]: No response from target!\r\n");
    }

    return bRetVal;
}

bool Bootloader_RequestEraseFlash(void)
{
    tsFrame tsSendMsg;

    tsSendMsg.u8ID = eService_eraseFlash;
    tsSendMsg.u16Length = 0;

    if (ComPort_SendGenericFrame(&tsSendMsg, 7000) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            printf("[Target]: Flash memory erased!\r\n");
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
        return false;
    }
}

bool Bootloader_RequestBootSession(uint16_t Fu16Timeout)
{
    tsFrame tsSendMsg;

    tsSendMsg.u8ID = eService_gotoBoot;
    tsSendMsg.u16Length = 2;
    tsSendMsg.pu8Payload[0] = (Fu16Timeout >> 8) & 0x00FF;
    tsSendMsg.pu8Payload[1] = Fu16Timeout & 0x00FF;

    if (ComPort_SendGenericFrame(&tsSendMsg, 50) == true)
    {
        if (tsSendMsg.pu8Response[0] == eOperationSuccess)
        {
            printf("[Target]: Boot session started!\r\n");
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
        return false;
    }
}
