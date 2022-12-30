#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_FRAME_LENGTH    262
#define BYTES_PER_BLOCK     256 /* which correspond to a flash row size in byte */
#define APPL_DESC_LEN       128
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
    uint16_t u16Lengh;
    uint8_t pu8Payload[MAX_FRAME_LENGTH];
} tsFrame;

typedef enum
{
    eService_gotoBoot =         0x01,
    eService_echo =             0x02,
    eService_getInfo =          0x03,
    eService_eraseFlash =       0x04,
    eService_dataTransfer =     0x05,
    eService_checkFlash =       0x06,
    eService_writePin =         0x0A,
    eService_readPin =          0x0B,
}teBootServices;

typedef enum
{
    eOperationSuccess =         0,
    eOperationNotAvailable =    1,
    eOperationNotAllowed =      2,
    eOperationFail =            3,
    eBadChecksum =              4,
    eBadFrameLength =           5,
    eUnknownFrameID =           6,
    eFrameTimeout =             7,
    eFlashEraseError =          8,
    eBadBlockAddr =             9,
    eBadCRCBlock =              10,
    eFlashWriteError =          11,
    eAppliCheckError =          12,
    eBootSessionTimeout =       13
}teOperationRetVal;

static uint8_t HexToDec(uint8_t h);
static void sendBlock(uint32_t Fu32NewStartAddr);
static void manageAppliDescription(void);

void initDatablockGen(void);
bool callback_dataManager(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
bool callback_findLogisticData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
void preParser(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len);
void getSwInfo(void);

#endif // CORE_H_INCLUDED
