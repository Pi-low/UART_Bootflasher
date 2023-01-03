#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Config.h"
#include "main.h"
#include "Bootloader/Bootloader.h"
#include "ComPort/ComPort.h"

uint32_t main_GetFileSize(FILE* FpHexFile);

int main(int argc, char * argv[])
{
    uint8_t u8KeepLoop = 1;
    FILE* MyFile;
    teMainStates teMainCurrentState = eStateSelectComPort;
    uint32_t u32TotalFileSize = 0;
    char* pcString = NULL;
    char pcBuffer[10];
    bool bTmp = true;
    while (u8KeepLoop)
    {
        switch(teMainCurrentState)
        {
            case eStateSelectComPort:
                ComPort_Scan();
                printf("Select port COM: \r\n");
                scanf("%[^\n]", pcBuffer);
                if (ComPort_Open(pcBuffer))
                {
                    teMainCurrentState = eStateSelectFile;
                }
                else
                {
                    u8KeepLoop = 0;
                }
            break;

            case eStateSelectFile:
                /* Catch filename argument */
                if ((argc > 1) && (argv[1] != NULL))
                {
                    //pcString = (char*) malloc(strlen(argv[1]) * sizeof(char));
                    pcString = argv[1];
                }
                else
                {
#ifdef DEBUG_CONFIG
                    pcString = (char*) malloc(strlen("test.hex") * sizeof(char));
                    sprintf(pcString, "test.hex");
#else
                    pcString = (char*) malloc(256 * sizeof(char));
                    printf("Select filename: \r\n");
                    scanf("%[^\n]", pcString);
#endif // DEBUG_CONFIG
                }
                MyFile = fopen(pcString, "rb");
                if (MyFile != NULL)
                {
                    printf("Open: \"%s\"\r\n", pcString);
                    u32TotalFileSize = main_GetFileSize(MyFile);
                    Bootloader_GetInfoFromiHexFile(MyFile, u32TotalFileSize);
#if DEBUG_OFFLINE == 1
                    teMainCurrentState = eStateFlashTarget;
                    system("PAUSE");
#else
                    teMainCurrentState = eStateTargetInfo;
#endif // DEBUG_OFFLINE
                }
                else
                {
                    printf("[INFO]: Cannot open file, exit program !\r\n");
                    u8KeepLoop = 0;
                }
                free(pcString);

            break;

            case eStateTargetInfo:
#if DEBUG_OFFLINE == 0
                printf("[INFO]: Request farget info:\r\n");
                bTmp &= Bootloader_RequestSwVersion(NULL);
                bTmp &= Bootloader_RequestSwInfo(NULL);
                printf("[INFO]: Abort operation !\r\n");
#endif // DEBUG_OFFLINE
                if (bTmp)
                {
                    teMainCurrentState = eStateFlashTarget;
                    system("PAUSE");
                }
                else
                {
                    u8KeepLoop = 0;
                }
            break;

            case eStateFlashTarget:
                if (MyFile != NULL)
                {
#if DEBUG_OFFLINE == 0
                    printf("[INFO]: Target flash erase\r\n");
                    bTmp &= Bootloader_RequestEraseFlash();
#endif // DEBUG_OFFLINE
                    if (bTmp)
                    {
                        Bootloader_ProcessFile(MyFile, u32TotalFileSize);
                    }
                    else
                    {
                        printf("[INFO]: Abort operation !\r\n");
                    }
                }
                u8KeepLoop = 0;
            break;

            default:
                u8KeepLoop = 0;
            break;
        }
    }

    if (MyFile != NULL)
    {
        fclose(MyFile);
    }
    ComPort_Close();
    #ifndef DEBUG_CONFIG
system("PAUSE");
    #endif // DEBUG_CONFIG
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
