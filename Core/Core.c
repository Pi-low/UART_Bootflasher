#include <stdio.h>
#include <string.h>
#include "../Config.h"
#include "../Bootloader/Bootloader.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "../Logger/Logger.h"
#include "Core.h"

/* ------------------------------------------------------------ */
/* Static variables declaration                                 */
/* ------------------------------------------------------------ */
const static uint8_t Pu8BlankWord[4] = {0xFF, 0xFF, 0xFF, 0x00};
static tsDataBlock tsCurrentDatablock;
static tsDataBlock tsCRCDatablock;

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
static bool Core_SendBlock(uint32_t Fu32NewStartAddr);
static void Core_FormatDecription(void);
static void Core_BlankDataBlock(tsDataBlock *FptsDataBlock);
static void Core_ManageCrcBlock(tsDataBlock *FptsDataBlock);

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

void Core_InitCRCBlockGen(void)
{
    tsCRCDatablock.u32StartAddr = 0;
    tsCRCDatablock.u32EndAddr = BYTES_PER_BLOCK;
    tsCRCDatablock.u16Len = 0;
}

bool Core_CbDataBlockGen(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size)
{
    bool bRetVal = true;
    uint16_t u16Cnt = 0;
    uint8_t pu8SaveData[BYTES_PER_BLOCK];
    uint8_t u8SaveLen = 0;
    uint32_t u32Fill = 0;
    if (Fpu8Buffer == NULL)
    {
        Core_SendBlock(0xFFFFFF00);
        return false;
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
                memcpy(&tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len], Pu8BlankWord, 4);
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
            bRetVal &= Core_SendBlock(tsCurrentDatablock.u32EndAddr); /* send block, reset block with new address */
            for (u16Cnt = 0; u16Cnt < u8SaveLen; u16Cnt++)
            {
                tsCurrentDatablock.pu8Data[tsCurrentDatablock.u16Len] = pu8SaveData[u16Cnt];
                tsCurrentDatablock.u16Len ++;
            }

        }
    }
    else if(Fu32addr >= tsCurrentDatablock.u32EndAddr)
    {
        /* Parsed address is over the current block */
        bRetVal &= Core_SendBlock(Fu32addr); /* send current block, reset block with parsed address */
        u16Cnt = 0;
        while (u16Cnt < tsCurrentDatablock.u16Len)
        {
            memcpy(&tsCurrentDatablock.pu8Data[u16Cnt], Pu8BlankWord, 4);
            u16Cnt += 4;
        }
        for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
        {
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

void Core_PreParse(uint8_t *Fpu8Buffer, uint32_t *Fpu32Len)
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

bool Core_SendBlock(uint32_t Fu32NewStartAddr)
{
   bool bRetVal = true;
   uint32_t u32OffsetAddr = 0;
   /* Send the block */
    bRetVal = Bootloader_TransferData(&tsCurrentDatablock);

    if (tsCurrentDatablock.u32StartAddr < ADDR_APPL_END)
    {
        Core_ManageCrcBlock(&tsCurrentDatablock);
    }

    /* Prepare next block */
    tsCurrentDatablock.u16CRCBlock = 0;
    u32OffsetAddr = Fu32NewStartAddr % (uint32_t)BYTES_PER_BLOCK;
    tsCurrentDatablock.u32StartAddr = Fu32NewStartAddr - u32OffsetAddr;
    tsCurrentDatablock.u32EndAddr = tsCurrentDatablock.u32StartAddr + (uint32_t)BYTES_PER_BLOCK;
    tsCurrentDatablock.u16Len = u32OffsetAddr;

   return bRetVal;
}

void Core_ManageCrcBlock(tsDataBlock *FptsDataBlock)
{
    static uint32_t u32PrevAddr = 0;
    char pcLogString[256];
    uint32_t u32CurStartAddr = FptsDataBlock->u32StartAddr;
    tsDataBlock *pBlock = FptsDataBlock;
    tsDataBlock tsBlankedBlock;
    uint8_t *Pu8Data = FptsDataBlock->pu8Data;
    uint16_t u16Cnt = 0;
    uint32_t u32AddrGap = 0;
    uint32_t u32AddrCnt = 0;

    /* Fill end of current block in case not */
    while (pBlock->u16Len < BYTES_PER_BLOCK)
    {
        memcpy(Pu8Data + pBlock->u16Len, Pu8BlankWord, 4);
        pBlock->u16Len += 4;
    }

    if (pBlock->u32StartAddr == 0)
    {
        /* In cas block address = 0, blanck the 2 first instructions (reset vectore & nop) */
        memcpy(Pu8Data, Pu8BlankWord, 4); /* Blank word 1 */
        memcpy(Pu8Data + 4, Pu8BlankWord, 4); /* Blank word 2 */
    }
/* ----------------------------------- */
/*    Manage CRC for current block     */
/* ----------------------------------- */
    if (u32PrevAddr == FptsDataBlock->u32StartAddr)
    {
        /* Boundary aligned block */
        Bootloader_ManageCrcData(pBlock);
        u32PrevAddr = pBlock->u32EndAddr;
    }
    else
    {
        tsBlankedBlock.u32StartAddr = u32PrevAddr;
        tsBlankedBlock.u32EndAddr = pBlock->u32StartAddr + BYTES_PER_BLOCK;

        if ((pBlock->u32StartAddr > u32PrevAddr) && (pBlock->u32StartAddr == ADDR_START_APPLI))
        {
            /* Add blanked block until bootloader start address */
            u32AddrGap = ADDR_START_BOOT - u32PrevAddr;
#if PRINT_DEBUG_TRACE && PRINT_BLOCK_CRC
            printf("<============================ [CRC]: Current block:%06X Prev:%06X Add blank: %u\r\n", pBlock->u32StartAddr, u32PrevAddr, u32AddrGap / BYTES_PER_BLOCK);
#endif
            sprintf(pcLogString, "CRC: Add %u blank row(s) from 0x%06X to 0x%06X\r", u32AddrGap / BYTES_PER_BLOCK, u32PrevAddr, ADDR_START_BOOT);
            Logger_Append(pcLogString);
            while (u32AddrCnt < u32AddrGap)
            {
                Core_BlankDataBlock(&tsBlankedBlock);
                Bootloader_ManageCrcData(&tsBlankedBlock);
                tsBlankedBlock.u32StartAddr += BYTES_PER_BLOCK;
                tsBlankedBlock.u32EndAddr += BYTES_PER_BLOCK;
                u32AddrCnt += BYTES_PER_BLOCK;
            }
#if PRINT_DEBUG_TRACE && PRINT_BLOCK_CRC
            printf("[CRC IVT = 0x%04X]\r\n", Bootloader_GetCrcData());
#endif
            sprintf(pcLogString, "CRC IVT=0x%04X\r", Bootloader_GetCrcData());
            Logger_Append(pcLogString);
        }
        else if (pBlock->u32StartAddr > u32PrevAddr)
        {
            /* Add blanked block until current block start address */
            u32AddrGap = pBlock->u32StartAddr - u32PrevAddr;
#if PRINT_DEBUG_TRACE && PRINT_BLOCK_CRC
            printf("<============================ [CRC]: Current block: 0x%06X Prev:0x%06X Add blank: %u\r\n", pBlock->u32StartAddr, u32PrevAddr, u32AddrGap / BYTES_PER_BLOCK);
#endif
            sprintf(pcLogString, "CRC: Add %u blank row(s) from 0x%06X to 0x%06X\r", u32AddrGap / BYTES_PER_BLOCK, u32PrevAddr, pBlock->u32StartAddr);
            Logger_Append(pcLogString);
            while (u32AddrCnt < u32AddrGap)
            {
                Core_BlankDataBlock(&tsBlankedBlock);
                Bootloader_ManageCrcData(&tsBlankedBlock);
                tsBlankedBlock.u32StartAddr += BYTES_PER_BLOCK;
                tsBlankedBlock.u32EndAddr += BYTES_PER_BLOCK;
                u32AddrCnt += BYTES_PER_BLOCK;
            }
        }
        else
        {
            Logger_Append("Warning : Core_ManageCrcBlock exception at 0x%06X\r");
            printf("[Warning]: Core_ManageCrcBlock exception at 0x%06X\r\n", pBlock->u32StartAddr);
            system("pause");
        }
        /* Start adding current block */
#if PRINT_DEBUG_TRACE && PRINT_BLOCK_CRC
        printf("[CRC] Resume with current block: 0x%06X Prev:0x%06X ================================>\r\n", pBlock->u32StartAddr, u32PrevAddr);
#endif
        Bootloader_ManageCrcData(pBlock);
        u32PrevAddr = pBlock->u32EndAddr;
    }
}

void Core_BlankDataBlock(tsDataBlock *FptsDataBlock)
{
    uint8_t *Pu8Data = FptsDataBlock->pu8Data;
    uint16_t u16Index = 0;
    while (u16Index < BYTES_PER_BLOCK)
    {
        memcpy(Pu8Data + u16Index, Pu8BlankWord, 4);
        u16Index += 4;
    }
    FptsDataBlock->u16Len = BYTES_PER_BLOCK;
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
    char pcLogString[512];
    sprintf(pcLogString, "\r      Hexfile SW Version : %02X.%02X\r      Hexfile SW Info : %s\r", u8SwMajor, u8SwMinor, pu8LogisticData);
    Logger_Append(pcLogString);
    printf("[File SW Version]: %02X.%02X \r\n[File SW Info]: %s\r\n", u8SwMajor, u8SwMinor, pu8LogisticData);
    if (Fpu16Version != NULL)
    {
        *(Fpu16Version) = (((uint16_t)u8SwMajor << 8) & 0xFF00) | (uint16_t)u8SwMajor;
    }
    if (Fpu8Decription != NULL)
    {
        memcpy(Fpu8Decription, pu8LogisticData, 128);
    }
}
