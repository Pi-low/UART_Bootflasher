#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Config.h"
#include "main.h"
#include "Bootloader/Bootloader.h"
#include "ComPort/ComPort.h"
#include "Logger/Logger.h"

uint32_t main_GetFileSize(FILE* FpHexFile);

int main(int argc, char * argv[])
{
    uint8_t u8KeepLoop = 1;
    FILE *MyFile;
    teMainStates teMainCurrentState = eStateSelectComPort;
    uint32_t u32TotalFileSize = 0;
    char *pcString = NULL;
    char pcBuffer[10];
    bool bTmp = true;
    while (u8KeepLoop)
    {
        switch(teMainCurrentState)
        {
            case eStateSelectComPort:
                ComPort_Scan();
                printf("Select port COM: \r\n");
                fflush(stdin);
                scanf("%[^\n]", pcBuffer);
                if (ComPort_Open(pcBuffer))
                {
                    teMainCurrentState = eStateSelectFile;
                }
                else
                {
                    teMainCurrentState = eStateQuit;
                }
            break;

            case eStateSelectFile:
                /* Catch filename argument */
                if ((argc > 1) && (argv[1] != NULL) && (pcString == NULL))
                {
                    pcString = argv[1];
                }
                else
                {
#ifdef DEBUG_CONFIG
                    pcString = (char*) malloc(strlen("test.hex") * sizeof(char));
                    sprintf(pcString, "test.hex");
#else
                    fflush(stdin);
                    pcString = (char*) malloc(256 * sizeof(char));
                    printf("Enter filename (**.hex): \r\n");
                    scanf("%[^\n]", pcString);
#endif // DEBUG_CONFIG
                }
                MyFile = fopen(pcString, "rb");
                if (MyFile != NULL)
                {
                    printf("Open: \"%s\"\r\n", pcString);
                    u32TotalFileSize = main_GetFileSize(MyFile);
                    Bootloader_GetInfoFromiHexFile(MyFile, u32TotalFileSize);
                    teMainCurrentState = eStateTargetInfo;
                }
                else
                {
                    printf("[Error]: Cannot open file, exit program !\r\n");
                    teMainCurrentState = eStateQuit;
                }
            break;

            case eStateTargetInfo:
                bTmp &= Bootloader_RequestSwVersion(NULL);
                bTmp &= Bootloader_RequestSwInfo(NULL);
                if (bTmp)
                {
                    teMainCurrentState = eStateFlashTarget;
                    fflush(stdin);
                    system("PAUSE");
                }
                else
                {
                    printf("[Error]: Abort operation !\r\n");
                    teMainCurrentState = eStateQuit;
                }
            break;

            case eStateFlashTarget:
                if (MyFile != NULL)
                {
                    bTmp &= Bootloader_RequestBootSession();
                    bTmp &= Bootloader_RequestEraseFlash();
                    if (bTmp)
                    {
                        Bootloader_ProcessFile(MyFile, u32TotalFileSize);
                    }
                    else
                    {
                        printf("[Error]: Abort operation !\r\n");
                    }
                }
                teMainCurrentState = eStateQuit;
            break;

            case eStateQuit:

                printf("Restart ? (y) : ");
                fflush(stdin);
                scanf("%[^\n]", pcBuffer);
                if (pcBuffer[0] == 'y')
                {
                    teMainCurrentState = eStateTargetInfo;
                }
                else
                {
                    u8KeepLoop = 0;
                    if (MyFile != NULL)
                    {
                        fclose(MyFile);
                    }
                    ComPort_Close();
                }
            break;

            default:
                u8KeepLoop = 0;
            break;
        }
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
