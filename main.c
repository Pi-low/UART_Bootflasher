#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Config.h"
#include "Core/Core.h"
#include "Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "RS-232/rs232.h"

bool main_Bootloading(FILE* FpHexFile, uint32_t Fu32FileSize, uint8_t* Fpu8DataBuffer);
bool main_GetInfoFromiHexFile(FILE* FpHexFile, uint32_t Fu32FileSize, uint8_t* Fpu8DataBuffer);
uint32_t main_GetFileSize(FILE* FpHexFile);

int main(int argc, char * argv[])
{
    FILE* MyFile;
    uint32_t u32TotalFileSize = 0;
    uint8_t pu8DataBuffer[ALLOCATION_SIZE + EXTENSION];
    char* cFilename = NULL;

    Core_InitDataBlockGen();

/* ================================================================== */
/*  MANAGE FILE                                                       */
/* ================================================================== */
    cFilename = (char *) malloc(strlen("test.hex") * sizeof(char));
    sprintf(cFilename, "test.hex");

    MyFile = fopen(cFilename, "rb");
    if (MyFile == NULL)
    {
        printf("Cannot open file, exit program !\r\n");
        system("PAUSE");
        return 0;
    }
    else
    {
        printf("Open: \"%s\"\r\n", cFilename);
    }

    u32TotalFileSize = main_GetFileSize(MyFile);
    main_GetInfoFromiHexFile(MyFile, u32TotalFileSize, pu8DataBuffer);

    system("PAUSE");

    main_Bootloading(MyFile, u32TotalFileSize, pu8DataBuffer);

    fclose(MyFile);
    system("PAUSE");
    return EXIT_SUCCESS;
}

bool main_Bootloading(FILE* FpHexFile, uint32_t Fu32FileSize, uint8_t* Fpu8DataBuffer)
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
        u32Read = fread(Fpu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE/BLOCK_SIZE), FpHexFile);
        if (u32Read != 0)
        {
            u32Read *= BLOCK_SIZE;
            u32DataCnt += u32Read;

            Core_PreParse(Fpu8DataBuffer, &u32Read);
            printf("Read %u:\r\n", u32Read);
            RetVal &= ihex_parser(Fpu8DataBuffer, u32Read);
        }
        else
        {
            fseek(FpHexFile, -(int)BLOCK_SIZE, SEEK_END); /* Set cursor to last block */
            u32Read = fread(Fpu8DataBuffer, BLOCK_SIZE, 1, FpHexFile);
            u32Remain = Fu32FileSize - u32DataCnt;
            if (u32Read != 0)
            {
                u32Read *= BLOCK_SIZE;
                u32DataCnt += u32Remain;

                pu8LastData = Fpu8DataBuffer;
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

bool main_GetInfoFromiHexFile(FILE* FpHexFile, uint32_t Fu32FileSize, uint8_t* Fpu8DataBuffer)
{
    bool RetVal = true;
    uint32_t u32Read = 0;

    ihex_reset_state();
    ihex_set_callback_func((ihex_callback_fp)Core_CbFetchLogisticData);
    fseek(FpHexFile, 0, SEEK_SET);

    do
    {
        u32Read = fread(Fpu8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE/BLOCK_SIZE), FpHexFile);
        u32Read *= BLOCK_SIZE;
        Core_PreParse(Fpu8DataBuffer, &u32Read);
    } while (ihex_parser(Fpu8DataBuffer, u32Read));
    Core_GetSwInfo(NULL, NULL);
    return RetVal;
}

uint32_t main_GetFileSize(FILE* FpHexFile)
{
    uint32_t u32FileSize = 0;
    if (fseek(FpHexFile, 0, SEEK_END) == 0)
    {
        u32FileSize = ftell(FpHexFile);
    }
    return u32FileSize;
}