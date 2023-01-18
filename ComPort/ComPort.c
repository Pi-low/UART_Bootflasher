#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "../RS-232/rs232.h"
#include "../Bootloader/Bootloader.h"
#include "../Logger/Logger.h"
#include "ComPort.h"

static int siComPortNumber = 0;
static bool sbComOpened = false;
static uint8_t spu8ListComPort[MAX_COM_PORT_NB];
static uint8_t sp2u8FrameFIFO[MAX_FRAME_FIFO_SIZE][MAX_BUFFER_SIZE];
static uint8_t spu8FrameFIFOIndex = 0;
static uint8_t spu8RxBuf[MAX_BUFFER_SIZE * 2];

static uint16_t ComPort_FetchIncomingFrames(int Fi16Timeout);

void ComPort_Scan(void)
{
    uint8_t u8Cnt = 0, u8ListCnt = 0;


    for (u8Cnt = 0; u8Cnt < MAX_COM_PORT_NB; u8Cnt++)
    {
        if (!RS232_OpenComport(u8Cnt, 115200,"8N1", 0))
        {
            spu8ListComPort[u8ListCnt] = u8Cnt;
            u8ListCnt++;
            RS232_CloseComport(u8Cnt);
        }
    }
    fflush(stdout);
    printf("Available COM ports:\r\n");
    for (u8Cnt = 0; u8Cnt < u8ListCnt; u8Cnt++)
    {
        printf("[%u] COM%u\r\n", u8Cnt, spu8ListComPort[u8Cnt] + 1);
    }
}

bool ComPort_Open(char* FpcString)
{
    bool bRetVal = true;
    char pcLogString[64];
    if ((*(FpcString) >= '0') && (*(FpcString) <= '9'))
    {
        siComPortNumber = atoi((const char*)FpcString);
        siComPortNumber = spu8ListComPort[siComPortNumber];
    }
    else
    {
        siComPortNumber = RS232_GetPortnr((const char*)FpcString);
    }
    if (RS232_OpenComport(siComPortNumber, 115200, "8N1", 0))
    {
        bRetVal = false;
    }
    else
    {
        sbComOpened = true;
        sprintf(pcLogString, "COM%u opened\r", spu8ListComPort[siComPortNumber]);
        Logger_Append(pcLogString);
    }
    return bRetVal;
}

void ComPort_Close(void)
{
    if (sbComOpened == true)
    {
        RS232_CloseComport(siComPortNumber);
    }
}

bool ComPort_SendGenericFrame(tsFrame* FptsMsg, uint16_t Fu16Timeout)
{
    bool bRetVal = true;
    uint16_t u16ByteCnt = 0;
    uint8_t u8Checksum = 0;
    uint8_t* pu8Tmp = NULL;
    //uint8_t pu8TxBuffer[MAX_BUFFER_SIZE];
    uint8_t *pu8RxBuffer = FptsMsg->pu8Response;
    uint16_t u16FrmLength = FptsMsg->u16Length + 1;
    uint16_t u16i;
    uint16_t u16NbRxFrm = 0;
    char pcLogString[2048];
    ComPort_SendStandaloneFrame(FptsMsg);
    // pu8TxBuffer[0] = 0xA5;
    // pu8TxBuffer[1] = FptsMsg->u8ID;
    // pu8TxBuffer[2] = u16FrmLength >> 8;
    // pu8TxBuffer[3] = u16FrmLength;

    // u8Checksum = pu8TxBuffer[1] + pu8TxBuffer[2] + pu8TxBuffer[3];

    // for (u16i = 0; u16i < FptsMsg->u16Length; u16i++)
    // {
    //     pu8TxBuffer[4 + u16i] = FptsMsg->pu8Payload[u16i];
    //     u8Checksum += FptsMsg->pu8Payload[u16i];
    // }

    // u8Checksum = ~u8Checksum;
    // pu8TxBuffer[4 + u16i] = u8Checksum;
    // RS232_flushRX(siComPortNumber);
    // RS232_SendBuf(siComPortNumber, pu8TxBuffer, u16FrmLength + 4);
    // sprintf(pcLogString, "UART Tx:\r");
    // Logger_AppendArray(pcLogString, pu8TxBuffer, u16FrmLength + 4);
// #if PRINT_DEBUG_TRACE
//     printf("[COM Tx]: ");
//     for(u16i = 0; u16i < u16FrmLength + 4; u16i++)
//     {
//         if ((u16i % 32) == 0)
//         {
//             printf("\r\n");
//         }
//         printf("%02X ", *(pu8TxBuffer + u16i));
//     }
//     printf("\r\n");
// #endif // PRINT_DEBUG_TRACE

/* =============================================================================== */
/*    RECEIVING                                                                    */
/* =============================================================================== */

    u16NbRxFrm = ComPort_FetchIncomingFrames((int)Fu16Timeout); /* <- Integrates blocking delay */
    if (u16NbRxFrm != 0)
    {
        for (u16i = 0; u16i < spu8FrameFIFOIndex; u16i++)
        {
            pu8Tmp = sp2u8FrameFIFO[u16i];
            if ((*pu8Tmp & 0x0F) == FptsMsg->u8ID)
            {
                u16FrmLength = (((uint16_t) *(pu8Tmp + 1) << 8) & 0xFF00) | (uint16_t) *(pu8Tmp + 2);
                u16FrmLength--; /* Escape checksum value */
                FptsMsg->u16Length = u16FrmLength;
                pu8Tmp += 3;
                memcpy(pu8RxBuffer, pu8Tmp, u16FrmLength);
            }
            else if (*pu8Tmp == 0)
            {
                /* Sporadic boot frame (info) */
                pu8Tmp += 3;
                Bootloader_PrintErrcode(*pu8Tmp);
            }
        }
        spu8FrameFIFOIndex = 0;
    }
    else
    {
        bRetVal = false;
        printf("[Error]: No response from device\r\n");
    }

    return bRetVal;
}

uint16_t ComPort_FetchIncomingFrames(int Fi16Timeout)
{
    uint16_t u16DataAmount = 0, u16FrameLen = 0, u16Cnt = 0, u16a = 0, u16RetVal = 0;
    uint8_t *pu8Buf = spu8RxBuf;
    uint8_t u8Checksum = 0;
    char pcLogString[1024];

    /* Receive data */
    do
    {
        Sleep(UNIT_TIME_DIV);
        Fi16Timeout -= UNIT_TIME_DIV;

        u16Cnt = RS232_PollComport(siComPortNumber, (unsigned char *)pu8Buf, MAX_FRAME_LENGTH);
        pu8Buf += u16Cnt;
        u16DataAmount += u16Cnt;
    } while ((Fi16Timeout > 0) && ((u16DataAmount == 0) || (u16Cnt != 0))); /* Read data until no data to read or timeout*/

    pu8Buf = spu8RxBuf; /* Restore pointer to buffer start address */

    /* Searching for frames */
    u16Cnt = 0;
    while ((u16Cnt < u16DataAmount) && (spu8FrameFIFOIndex < MAX_FRAME_FIFO_SIZE))
    {
        if (*pu8Buf == 0x5A) /* Start of frame */
        {
            pu8Buf++; /* escape SoF byte */
            u16Cnt++;
            u16FrameLen = (((uint16_t) * (pu8Buf + 1) << 8) & 0xFF00) | (uint16_t) * (pu8Buf + 2);
            if ((u16FrameLen < MAX_FRAME_LENGTH) && ((u16Cnt + u16FrameLen + 3) <= u16DataAmount))
            {
                for (u16a = 0; u16a < u16FrameLen + 3; u16a++)
                {
                    u8Checksum += *(pu8Buf + u16a);
                }
                if (u8Checksum == 0xff)
                {
                    memcpy(sp2u8FrameFIFO[spu8FrameFIFOIndex], pu8Buf, u16FrameLen + 3);
                    u16Cnt += u16FrameLen + 3;
                    spu8FrameFIFOIndex++;
                    u16RetVal++;
                }
                else
                {
                    /* Error: wrong checksum */
                    printf("[Error]: wrong checksum\r\n");
                    sprintf(pcLogString, "UART Rx: bad checksum !\r");
                    Logger_Append(pcLogString);
                }
            }
            else
            {
                /* Error: wrong frame length */
                printf("[Error]: incorrect length\r\n");
                sprintf(pcLogString, "UART Rx: bad frame length !\r");
                Logger_Append(pcLogString);
            }
        }
        else
        {
            while ((*pu8Buf != 0x5A) && (u16Cnt < u16DataAmount))
            {
                pu8Buf++;
                u16Cnt++;
            }
        }
    }
#if PRINT_DEBUG_TRACE
        printf("[COM Rx]: \r\n");
    for (u16a = 0; u16a < u16DataAmount; u16a++)
    {
        if (((u16a % 32) == 0) && (u16a != 0))
        {
            printf("\r\n");
        }
        printf("%02X ", *(spu8RxBuf + u16a));
    }
    printf("\r\n found %u\r\n", u16RetVal);
#endif // PRINT_DEBUG_TRACE
    sprintf(pcLogString, "UART Rx (%u):\r", u16DataAmount);
    Logger_AppendArray(pcLogString, spu8RxBuf, u16DataAmount);
    if (spu8FrameFIFOIndex == MAX_FRAME_FIFO_SIZE)
    {
        printf("[Error]: Rx Frame FIFO overflow\r\n");
        sprintf(pcLogString, "UART Rx: FIFO overflow !\r");
        Logger_Append(pcLogString);
    }
    return u16RetVal;
}

void ComPort_SendStandaloneFrame(tsFrame* FptsMsg)
{
    uint8_t u8pBufTx[MAX_FRAME_LENGTH];
    uint16_t u16Cnt = 0;
    uint8_t u8Checksum = 0;
    uint16_t u16FrmLength = FptsMsg->u16Length + 5;
    char pcLogString[1024];

    u8pBufTx[0] = 0xA5;
    u8pBufTx[1] = FptsMsg->u8ID;
    u8pBufTx[2] = (FptsMsg->u16Length + 1) >> 8;
    u8pBufTx[3] = FptsMsg->u16Length + 1;
    memcpy(u8pBufTx + 4, FptsMsg->pu8Payload, FptsMsg->u16Length);

    for (u16Cnt = 1; u16Cnt < FptsMsg->u16Length + 3; u16Cnt++)
    {
        u8Checksum += u8pBufTx[u16Cnt];
    }
    u8pBufTx[FptsMsg->u16Length + 4] = ~u8Checksum;

#if PRINT_DEBUG_TRACE
    printf("[COM Tx]: ");
    for(u16Cnt = 0; u16Cnt < u16FrmLength; u16Cnt++)
    {
        if ((u16Cnt % 32) == 0 && (u16Cnt != 0))
        {
            printf("\r\n");
        }
        printf("%02X ", *(u8pBufTx + u16Cnt));
    }
    printf("\r\n");
#endif // PRINT_DEBUG_TRACE
    sprintf(pcLogString, "UART Tx:\r");
    Logger_AppendArray(pcLogString, u8pBufTx, u16FrmLength);
    RS232_flushRX(siComPortNumber);
    RS232_SendBuf(siComPortNumber, u8pBufTx, FptsMsg->u16Length + 5);
}

void ComPort_WaitForStartupSequence(uint16_t Fu16Timeout)
{
    uint16_t u16TimeCount = 0, u16DataAmount = 0, u16FrameLen = 0, u16Cnt = 0;
    uint8_t *pu8Buf = spu8RxBuf;
    uint8_t u8Checksum = 0;
    uint8_t u8ExitLoop = 0;
    tsFrame tsMsg;
    char pcLogString[1024];
    printf("Please reset the device within %us\r\nWaiting for startup sequence...\r\n", Fu16Timeout / 1000);
    sprintf(pcLogString, "Tool: Waiting for startup sequence\r");
    Logger_Append(pcLogString);
    tsMsg.u8ID = eService_echo;
    sprintf(tsMsg.pu8Payload, "Echo!");
    tsMsg.u16Length = strlen(tsMsg.pu8Payload);
    /* Receive data */
    do
    {
        /* Tight timing because boot send attention message every 50ms */
        Sleep(10);
        Fu16Timeout -= 10;
        if (Fu16Timeout % 1000 == 0)
        {
            printf(". ");
        }

        u16Cnt = RS232_PollComport(siComPortNumber, (unsigned char *)pu8Buf, MAX_FRAME_LENGTH);
        pu8Buf += u16Cnt;
        u16DataAmount += u16Cnt;
    } while ((Fu16Timeout > 0) && ((u16DataAmount == 0) || (u16Cnt != 0))); /* Read data until no data to read or timeout*/
    printf("\r\n");

    pu8Buf = spu8RxBuf; /* Restore pointer to buffer start address */
    sprintf(pcLogString, "UART Rx:\r");
    Logger_AppendArray(pcLogString, spu8RxBuf, u16DataAmount);
    /* Searching for frames */
    u16Cnt = 0;
    if (u16DataAmount != 0)
    {
        while ((u16Cnt < u16DataAmount) && (spu8FrameFIFOIndex < MAX_FRAME_FIFO_SIZE) && (!u8ExitLoop))
        {
            if (*pu8Buf == 0x5A) /* Start of frame */
            {
                pu8Buf++; /* escape SoF byte */
                u16Cnt++;
                u16FrameLen = (((uint16_t)pu8Buf[1] << 8) & 0xFF00) | pu8Buf[2];
                if (pu8Buf[0] == 0x00)
                {
                    if ((u16FrameLen == 2) && pu8Buf[3] == eBootAttention)
                    {
                        u8ExitLoop = 1;
                    }
                    else
                    {
                        pu8Buf += u16FrameLen + 3;
                        u16Cnt += u16FrameLen + 3;
                    }
                }
            }
            else
            {
                while ((*pu8Buf != 0x5A) && (u16Cnt < u16DataAmount))
                {
                    pu8Buf++;
                    u16Cnt++;
                }
            }
        }
        if (u8ExitLoop)
        {
            ComPort_SendStandaloneFrame(&tsMsg);
            printf("[Info] Startup sequence received !\r\n");
            sprintf(pcLogString, "Tool: Startup sequence received !\r");
            Logger_Append(pcLogString);
        }
        else
        {
            printf("[Info] Startup sequence not received !\r\n");
            sprintf(pcLogString, "Tool: Startup sequence not received !\r");
            Logger_Append(pcLogString);
        }
    }
}
