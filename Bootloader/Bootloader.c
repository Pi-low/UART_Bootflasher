#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "../Core/Core.h"
#include "../Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "Bootloader.h"

static uint8_t spu8DataBuffer[ALLOCATION_SIZE + EXTENSION];

bool Bootloader_ProcessFile(FILE* FpHexFile, uint32_t Fu32FileSize)
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
            printf("Read %u:\r\n", u32Read);
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
                printf("Read %u(%u): ", u32Read, u32Remain);
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
