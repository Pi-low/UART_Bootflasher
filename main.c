#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Config.h"
#include "Core/Core.h"
#include "Intel_HEX_parser/ihex_parser/ihex_parser.h"
#include "Crc16/Crc16.h"
#include "RS-232/rs232.h"

int main(int argc, char * argv[])
{
    FILE* MyFile;
    uint8_t u8DataBuffer[ALLOCATION_SIZE + 256];
    uint8_t* pu8LastData = NULL;
    bool bParseRes = true;
    uint32_t u32Read = 0;
    uint32_t u32lastRead = 0;
    uint32_t u32TotalFileSize = 0;
    uint32_t u32StreamBlockSize = 0;
    uint32_t u32DataCnt = 0;
    uint32_t u32Remain = 0;
    char* cFilename = NULL;

    ihex_set_callback_func((ihex_callback_fp)printHexData);
    ihex_reset_state();
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

    /* Start analysis */
    if (fseek(MyFile, 0, SEEK_END) == 0)
    {
        u32TotalFileSize = ftell(MyFile);
        fseek(MyFile, 0, SEEK_SET); /* Reset cursor */
        printf("File size: %u\r\n", u32TotalFileSize);
    }
    else
    {
        printf("Error fseek\r\n");
        system("PAUSE");
        return 0;
    }

    system("PAUSE");

    while (u32DataCnt < u32TotalFileSize)
    {
        u32Read = fread(u8DataBuffer, BLOCK_SIZE, (ALLOCATION_SIZE/BLOCK_SIZE), MyFile);
        if (u32Read != 0)
        {
            u32Read *= BLOCK_SIZE;
            u32DataCnt += u32Read;

            preParser(u8DataBuffer, &u32Read);
            bParseRes = ihex_parser(u8DataBuffer, u32Read);
            if (!bParseRes)
            {
                ihex_reset_state();
            }
            printf("Read %u (parsed: %u)\r\n", u32Read, bParseRes);
        }
        else
        {
            fseek(MyFile, -(int)BLOCK_SIZE, SEEK_END); /* Set cursor to last block */
            u32Read = fread(u8DataBuffer, BLOCK_SIZE, 1, MyFile);
            u32Remain = u32TotalFileSize - u32DataCnt;
            if (u32Read != 0)
            {
                u32Read *= BLOCK_SIZE;
                u32DataCnt += u32Remain;

                pu8LastData = u8DataBuffer;
                pu8LastData += u32Read - u32Remain;

                preParser(pu8LastData, &u32Remain);
                bParseRes = ihex_parser(pu8LastData, u32Remain);


                printf("Read %u(%u)(parsed: %u): ", u32Read, u32Remain, bParseRes);
            }
            else
            {
                printf("Read error, abort !\r\n");
                break; /* Get out of the while loop */
            }
        }
    }
    fclose(MyFile);
    system("PAUSE");
    return 0;
}
