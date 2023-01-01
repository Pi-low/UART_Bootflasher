#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Config.h"
#include "Bootloader/Bootloader.h"
#include "ComPort/ComPort.h"

uint32_t main_GetFileSize(FILE* FpHexFile);

int main(int argc, char * argv[])
{
    FILE* MyFile;
    uint32_t u32TotalFileSize = 0;
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
    Bootloader_GetInfoFromiHexFile(MyFile, u32TotalFileSize);
    ComPort_Scan();

    system("PAUSE");

    Bootloader_ProcessFile(MyFile, u32TotalFileSize);

    fclose(MyFile);
    system("PAUSE");
    return EXIT_SUCCESS;
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
