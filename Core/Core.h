#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define BYTES_PER_BLOCK 256 /* which correspond to a flash row size in byte */
#define APPL_DESC_LEN       128
#define ADDR_START_APPLI    (0x4000L << 1)
#define ADDR_APPL_DESC      (0x280L << 1)
#define ADDR_APPL_VERSION   (0x300L << 1)

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
    uint16_t u16Lengh;
    uint8_t pu8Payload;
    uint8_t u8Checksum;
} tsFrame;

static uint8_t HexToDec(uint8_t h);
static void sendBlock(uint32_t Fu32NewStartAddr);
static void manageAppliDescription(void);

void initDatablockGen(void);
bool dataManager(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
bool findLogisticData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
void preParser(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len);
void getSwInfo(void);

#endif // CORE_H_INCLUDED
