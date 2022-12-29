#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BYTES_PER_BLOCK 256

typedef struct
{
    uint32_t u32Addr;
    uint8_t pu8Data[BYTES_PER_BLOCK];
    uint16_t u16Index;
} tsDataBlock;

typedef enum
{
    eStateNewBlock = 0,
    eStateFillBlock = 1,
    eStatePushBlock = 2
} teBlockState;

static uint8_t HexToDec(uint8_t h);

void initDatablockGen(void);
bool dataManager(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
void preParser(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len);

#endif // CORE_H_INCLUDED
