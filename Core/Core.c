#include <stdio.h>
#include "Core.h"
#include "../Config.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"

static tsDataBlock;
static teBlockState teCurrentState;

static uint8_t HexToDec(uint8_t h)
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

void initDatablockGen(void)
{
    tsDataBlock.u32Addr = 0;
    tsDataBlock.u16Index = 0;
    teCurrentState = eStateNewBlock;
}

bool dataManager(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size)
{
    bool bRetVal = true;
    uint16_t u16Cnt = 0;
    uint16_t u16BlankSize = 0;
    uint16_t u16SavedLen = 0;
    uint8_t u8Save[BYTES_PER_BLOCK];
    static uint32_t u32MaxAddr = 0;

    switch(teCurrentState)
    {
        case eStateNewBlock:
            tsDataBlock.u32Addr = Fu32addr;
            tsDataBlock.u16Index = 0;
            u32MaxAddr = Fu32addr + BYTES_PER_BLOCK;
            for(u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
            {
                tsDataBlock.pu8Data[tsDataBlock.u16Index] = *(Fpu8Buffer + u16Cnt);
                tsDataBlock.u16Index++;
            }
            teCurrentState = eStateFillBlock;
        break;

        case eStateFillBlock:
            if (Fu32addr == (tsDataBlock.u16Index + tsDataBlock.u32Addr))  /* Continuous data */
            {
                for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
                {
                    if (tsDataBlock.u16Index >= BYTES_PER_BLOCK)
                    {
                        /* Block overflow */
                        u8Save[u16SavedLen] = *(Fpu8Buffer + u16Cnt);
                        u16SavedLen++;
                    }
                    else
                    {
                        tsDataBlock.pu8Data[tsDataBlock.u16Index] = *(Fpu8Buffer + u16Cnt);
                        tsDataBlock.u16Index++;
                    }
                }
                if (tsDataBlock.u16Index == BYTES_PER_BLOCK)
                {
                    teCurrentState = eStatePushBlock;
                }
            }
            else if ((Fu32addr > (tsDataBlock.u16Index + tsDataBlock.u32Addr)) && (Fu32addr < u32MaxAddr)) /* Address jump inside current block */
            {
                u16BlankSize = Fu32addr - tsDataBlock.u32Addr;
                while (u16Cnt < u16BlankSize)
                {
                    tsDataBlock.pu8Data[tsDataBlock.u16Index] = 0xFF;
                    tsDataBlock.pu8Data[tsDataBlock.u16Index + 1] = 0xFF;
                    tsDataBlock.pu8Data[tsDataBlock.u16Index + 2] = 0xFF;
                    tsDataBlock.pu8Data[tsDataBlock.u16Index + 3] = 0;
                    tsDataBlock.u16Index += 4;
                    u16Cnt += 4;
                }
                for (u16Cnt = 0; u16Cnt < Fu8Size; u16Cnt++)
                {
                    if (tsDataBlock.u16Index >= BYTES_PER_BLOCK)
                    {
                        /* Block overflow */
                        u8Save[u16SavedLen] = *(Fpu8Buffer + u16Cnt);
                        u16SavedLen++;
                    }
                    else
                    {
                        tsDataBlock.pu8Data[tsDataBlock.u16Index] = *(Fpu8Buffer + u16Cnt);
                        tsDataBlock.u16Index++;
                    }
                }
                if (tsDataBlock.u16Index == BYTES_PER_BLOCK)
                {
                    teCurrentState = eStatePushBlock;
                }
            }
            else if (Fu32addr > u32MaxAddr)
            {
                for(u16SavedLen = 0; u16SavedLen < Fu8Size; u16SavedLen++)
                {
                    u8Save[u16SavedLen] = *(Fpu8Buffer + u16SavedLen);
                }
                teCurrentState = eStatePushBlock;
            }

        case eStatePushBlock:

            break;
    }

    return bRetVal;
}

void preParser(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len)
{
    static uint32_t su32SavedLen = 0;
    uint32_t u32Index = 0;
    uint32_t u32PrevLineStart = 0;
    uint32_t u32Tmp = 0;

    static uint8_t pu8Saved[EXTENSION];
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
            u8Len = ((HexToDec(*(pu8char + 1)) << 4) & 0xF0) | (HexToDec(*(pu8char + 2)) & 0xF);
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
