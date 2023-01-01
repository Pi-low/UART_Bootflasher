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
    int iMainReturn = EXIT_SUCCESS;
    FILE* MyFile;
    teMainStates teToolState = eStateSelectComPort;
    uint32_t u32TotalFileSize = 0;
    char* cFilename = NULL;

    switch(teToolState)
    {
        case eStateSelectComPort:
        ComPort_Scan();
        break;

        case eStateSelectFile:
        if ((argc > 1) && (argv[1] != NULL))
        {
            cFilename = (char*) malloc(strlen(argv[1]) * sizeof(char));
        }
        else
        {
            cFilename = (char*) malloc(strlen("test.hex") * sizeof(char));
            sprintf(cFilename, "test.hex");
        }

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
            u32TotalFileSize = main_GetFileSize(MyFile);
            Bootloader_GetInfoFromiHexFile(MyFile, u32TotalFileSize);
        }
        break;

        case eStateTargetInfo:
        break;

        case eStateFlashTarget:
        Bootloader_ProcessFile(MyFile, u32TotalFileSize);
        break;

        case eStateQuit:
        if (MyFile != NULL)
        {
            fclose(MyFile);
        }
        break;

        default:
        break;
    }
    
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
