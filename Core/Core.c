#include <stdio.h>
#include "Core.h"
#include "../Config.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"

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

bool printHexData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size)
{
    uint8_t u8Cnt = 0;
    printf("[0x%06X:%u] ", Fu32addr, Fu8Size);
    for (u8Cnt = 0; u8Cnt < Fu8Size; u8Cnt++)
    {
        printf("%02X ", Fpu8Buffer[u8Cnt]);
    }
    printf("\r\n");
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
    uint8_t* pu8Parse = NULL;
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
        pu8Parse = pu8char;
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
                    if (ihex_parser(pu8Parse, u8Len) == false)
                    {
                        ihex_reset_state();
                    }
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
