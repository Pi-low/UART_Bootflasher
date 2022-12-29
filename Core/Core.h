#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct
{
    uint8_t u8Len;
    uint16_t u16Addr;

} tsIntelHEXline;

static uint8_t HexToDec(uint8_t h);

bool printHexData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
void preParser(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len);

#endif // CORE_H_INCLUDED
