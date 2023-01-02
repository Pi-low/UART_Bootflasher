#include <stdio.h>
#include "../Config.h"
#include "../Bootloader/Bootloader.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "../Crc16/Crc16.h"
#include "Core.h"

/* ------------------------------------------------------------ */
/* Static variables declaration                                 */
/* ------------------------------------------------------------ */
static tsDataBlock tsCurrentDatablock;

static uint8_t pu8LogisticData[128];
static uint8_t u8SwMajor = 0;
static uint8_t u8SwMinor = 0;
static uint8_t pu8Buff[256];
static uint16_t u16BufLen = 0;
static uint8_t pu8Saved[EXTENSION];
static uint32_t su32SavedLen = 0;

/* ------------------------------------------------------------ */
/* Static functions declaration                                 */
/* ------------------------------------------------------------ */
static uint8_t Core_HexToDec(uint8_t h);
static void Core_SendBlock(uint32_t Fu32NewStartAddr);
static void Core_FormatDecription(void);

uint8_t Core_HexToDec(uint8_t h)
{
    if (h >= '0' && h <= '9')
        return h - '0';
    else if (h >= 'A' && h <= 'F')
        return h - 'A' + 0xA;
    else if (h >= 'a' && h <= 'z')
        return h - 'a' + 0xA;
    else
        return 0;
}

void Core_InitDataBlockGen(void)
{
    tsCurrentDatablock.u32StartAddr = 0;
    tsCurrentDatablock.u32EndAddr = BYTES_PER_BLOCK;
    tsCurrentDatablock.u16Len = 0;
    su32SavedLen = 0;
}

bool Core_CbDataBlockGen(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size)
{
    bool bRetVal = true;
    uint16_t u16Cnt = 0;
    uint8_t pu8SaveData[BYTES_PER_BLOCK];
    uint8_t u8SaveLen = 0;
    uint32_t u32Fill = 0;

    if (Fpu8Buffer == NULL) /* Shall be used to send the last block which size is below 256 */
    {
        Core_SendBlock(0xFFFFFF00);
    }

    if ((Fu32addr >= tsCurrentDatablock.u32StartAddr) && (Fu32addr < tsCurrentDatablock.u32EndAddr))
    {
        /* Parsed address is withing the current block */
        if (Fu32addr == (tsCurrentDatablock.u32StartAddr + tsCurrentDatablock.u16Len))
        {
            /* Continuous data within block */
            for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
            {
                if (tsCurrentDatablock.u16Len >= BYTES_PER_BLOCK)
                {
                    /* Parsed data exceed the current block size, save data for next block */
                    pu8SaveData[u8SaveLen] = *(Fpu8Buffer + u16Cnt);
                    u8SaveLen++;
                }
                else
                {
                    /* Append datablock */
                    tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len] = *(Fpu8Buffer + u16Cnt);
                    tsCurrentDatablock.u16Len ++;
                }
            }
        }
        else if (Fu32addr >= (tsCurrentDatablock.u32StartAddr + tsCurrentDatablock.u16Len))
        {
            /* Data still within the current block  */
            u32Fill = Fu32addr - (tsCurrentDatablock.u32StartAddr + tsCurrentDatablock.u16Len);
            while (u16Cnt < u32Fill)
            {
                tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len] = 0xFF;
                tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len + 1] = 0xFF;
                tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len + 2] = 0xFF;
                tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len + 3] = 0; /* Phantom byte */
                u16Cnt += 4;
                tsCurrentDatablock.u16Len += 4;
            }
            for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
            {
                if (tsCurrentDatablock.u16Len >= BYTES_PER_BLOCK)
                {
                    /* Parsed data exceed the current block size, save data for next block */
                    pu8SaveData[u8SaveLen] = *(Fpu8Buffer + u16Cnt);
                    u8SaveLen++;
                }
                else
                {
                    /* Append datablock */
                    tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len] = *(Fpu8Buffer + u16Cnt);
                    tsCurrentDatablock.u16Len ++;
                }
            }
        }
        else
        {
            /* Fu32addr is lower than current index*/
            bRetVal = false;
            printf("Lower address: abort [%08X:%u]\r\n", Fu32addr, Fu8Size);
        }

        if((tsCurrentDatablock.u32StartAddr + tsCurrentDatablock.u16Len) >= tsCurrentDatablock.u32EndAddr)
        {
            /* Reach block end address, send block and load remaining data */
            Core_SendBlock(tsCurrentDatablock.u32EndAddr); /* send block, reset block with new address */
            for (u16Cnt = 0; u16Cnt < u8SaveLen; u16Cnt++)
            {
                tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len] = pu8SaveData[u16Cnt];
                tsCurrentDatablock.u16Len ++;
            }

        }
    }
    else if((Fu32addr >= tsCurrentDatablock.u32EndAddr) && (Fu32addr < ADDR_APPL_END))
    {
        /* Parsed address is over the current block */
        Core_SendBlock(Fu32addr); /* send current block, reset block with parsed address */
        for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
        {
            /* Append datablock */
            tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len] = *(Fpu8Buffer + u16Cnt);
            tsCurrentDatablock.u16Len ++;
        }
    }
    else
    {
        bRetVal = false;
    }

    return bRetVal;
}

bool Core_CbFetchLogisticData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size)
{
    bool bRetVal = true;
    uint16_t u16Cnt = 0;

    if ((Fu32addr >= ADDR_APPL_DESC) && (Fu32addr < ADDR_APPL_VERSION))
    {
        for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
        {
            pu8Buff[u16BufLen] = *(Fpu8Buffer + u16Cnt);
            u16BufLen++;
        }
        if (u16BufLen == BYTES_PER_BLOCK)
        {
            Core_FormatDecription();
        }
    }
    else if (Fu32addr == ADDR_APPL_VERSION)
    {
        for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
        {
            pu8Buff[u16BufLen] = *(Fpu8Buffer + u16Cnt);
            u16BufLen++;
        }
        u8SwMinor = pu8Buff[0];
        u8SwMajor = pu8Buff[1];
        bRetVal = false;
    }
    if (Fu32addr >= ADDR_START_APPLI)
    {
        bRetVal = false;
    }
    return bRetVal;
}

void Core_PreParse(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len)
{
    uint32_t u32Index = 0;
    uint32_t u32PrevLineStart = 0;
    uint32_t u32Tmp = 0;


    uint8_t u8Tmp = 0;
    uint8_t u8Len = 0;
    uint8_t* pu8char = NULL;
    uint8_t pu8TmpBuf[ALLOCATION_SIZE];

    if (su32SavedLen != 0)
    {
        for (u32Tmp = 0; u32Tmp < *Fpu32Len; u32Tmp++)
        {
            pu8TmpBuf[u32Tmp] = *(Fpu8Buffer + u32Tmp);
        }

        for (u8Tmp = 0; u8Tmp < su32SavedLen; u8Tmp++)
        {
            *(Fpu8Buffer + u8Tmp) = pu8Saved[u8Tmp];
        }
        u32Tmp = 0;

        while (u32Tmp < *Fpu32Len)
        {
            *(Fpu8Buffer + u8Tmp + u32Tmp) = pu8TmpBuf[u32Tmp];
            u32Tmp++;
        }
        *Fpu32Len += su32SavedLen;
        su32SavedLen = 0;
    }

    while (u32Index < *Fpu32Len)
    {
        pu8char = Fpu8Buffer + u32Index;
        if (*(pu8char) == ':')
        {
            u8Len = ((Core_HexToDec(*(pu8char + 1)) << 4) & 0xF0) | (Core_HexToDec(*(pu8char + 2)) & 0xF);
            u8Len <<= 1;
            u8Len += 11;
            if ((u32Index + u8Len + 2) < *(Fpu32Len))
            {
                pu8char += u8Len;
                if (*(pu8char) == '\r' && *(pu8char + 1) == '\n')
                {
                    u8Len += 2;
                    u32Index += u8Len;
                    u32PrevLineStart = u32Index;
                }
            }
            else
            {
                su32SavedLen = *Fpu32Len - u32PrevLineStart;

                for (u8Tmp = 0; u8Tmp < su32SavedLen; u8Tmp++)
                {
                    pu8Saved[u8Tmp] = *(pu8char + u8Tmp);
                }

                *Fpu32Len = u32PrevLineStart;
            }
        }
    }
}

void Core_SendBlock(uint32_t Fu32NewStartAddr)
{
   tsFrame tsDataTransferFrame;
   uint16_t u16Cnt = 0;
   uint16_t u16Tmp = 0;
   static uint16_t u16BlockCnt = 0;
   uint32_t u32OffsetAddr = 0;
   /* Use short variable name: lazy */
   uint8_t* pu8FrameData = tsDataTransferFrame.pu8Payload;
   uint16_t* pu16CRC = &tsCurrentDatablock.u16CRCBlock;

   tsDataTransferFrame.u16Length = tsCurrentDatablock.u16Len + 5;
   tsDataTransferFrame.u8ID = eService_dataTransfer;

   /* Load block address */
   *(pu8FrameData) =     (tsCurrentDatablock.u32StartAddr >> 16) & 0x000000FF;
   *(pu8FrameData + 1) = (tsCurrentDatablock.u32StartAddr >> 8) & 0x000000FF;
   *(pu8FrameData + 2) = tsCurrentDatablock.u32StartAddr & 0x000000FF;

   Crc16_BufferUpdate(pu16CRC, pu8FrameData, 3);
   pu8FrameData += 3;

   for (u16Cnt = 0; u16Cnt < tsCurrentDatablock.u16Len; u16Cnt++)
   {
       *(pu8FrameData + u16Cnt) = tsCurrentDatablock.pu8Data[u16Cnt];
   }

   Crc16_BufferUpdate(pu16CRC, pu8FrameData, tsCurrentDatablock.u16Len);
   pu8FrameData += tsCurrentDatablock.u16Len;

   /* XOR CRC16 */
   u16Tmp = *(pu16CRC) ^ 0xFFFF;
   /* Reverse CRC16 MSB/LSB */
   *(pu8FrameData) = u16Tmp & 0x00FF;
   *(pu8FrameData + 1) = (u16Tmp >> 8) & 0x00FF;

#ifdef PRINT_DEBUG_TRACE
   printf("Block %u\r\n", u16BlockCnt);
   for (u16Cnt = 0; u16Cnt < tsDataTransferFrame.u16Length; u16Cnt++)
   {
       if ((u16Cnt % 32) == 0)
       {
           printf("\r\n ");
       }
       if ((u16Cnt == 0) || (u16Cnt == (tsDataTransferFrame.u16Length - 2)))
       {
           printf("[");
       }
       printf("%02X", tsDataTransferFrame.pu8Payload[u16Cnt]);
       if ((u16Cnt == 2) || (u16Cnt == (tsDataTransferFrame.u16Length - 1)))
       {
           printf("]");
       }
       printf(" ");
   }
   printf("\r\n");
#endif
   /* Send the block */
   ComPort_SendGenericFrame(&tsDataTransferFrame, 50);

   /* Prepare next block */
   tsCurrentDatablock.u16CRCBlock = 0;
   if (Fu32NewStartAddr % BYTES_PER_BLOCK == 0)
   {
       tsCurrentDatablock.u32StartAddr = Fu32NewStartAddr;
       tsCurrentDatablock.u32EndAddr = tsCurrentDatablock.u32StartAddr + BYTES_PER_BLOCK;
       tsCurrentDatablock.u16Len = 0;
   }
   else
   {
       u32OffsetAddr = Fu32NewStartAddr % (uint32_t)BYTES_PER_BLOCK;
       tsCurrentDatablock.u32StartAddr = Fu32NewStartAddr - u32OffsetAddr;
       tsCurrentDatablock.u32EndAddr = tsCurrentDatablock.u32StartAddr + (uint32_t)BYTES_PER_BLOCK;
       tsCurrentDatablock.u16Len = 0;
   }
   u16BlockCnt ++;
}

void Core_FormatDecription(void)
{
    uint16_t u16Cnt = 0;
    uint16_t u16Index = 0;
    while (u16Cnt < u16BufLen)
    {
        pu8LogisticData[u16Index] = pu8Buff[u16Cnt];
        u16Index ++;
        pu8LogisticData[u16Index] = pu8Buff[u16Cnt + 1];
        u16Index ++;
        u16Cnt += 4;
    }
    u16BufLen = 0;
}

void Core_GetSwInfo(uint16_t* Fpu16Version, uint8_t* Fpu8Decription)
{
    printf("Found: SW %u.%u -> \"%s\"\r\n", u8SwMajor, u8SwMinor, pu8LogisticData);
    if (Fpu16Version != NULL)
    {
        *(Fpu16Version) = (((uint16_t)u8SwMajor << 8) & 0xFF00) | (uint16_t)u8SwMajor;
    }
    if (Fpu8Decription != NULL)
    {
        Fpu8Decription = pu8LogisticData;
    }
}
