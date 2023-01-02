#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include "../RS-232/rs232.h"
#include "../Bootloader/Bootloader.h"
#include "ComPort.h"

static int siComPortNumber = 0;
static bool sbComOpened = false;
static uint8_t spu8ListComPort[MAX_COM_PORT_NB];

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
    bool bErrCheck = true;
    uint16_t u16ByteCnt = 0;
    uint8_t u8Checksum = 0;
    uint8_t* pu8Tmp = NULL;
    uint8_t pu8TxBuffer[MAX_BUFFER_SIZE];
    uint8_t pu8RxBuffer[MAX_BUFFER_SIZE];
    uint16_t u16FrmLength = FptsMsg->u16Length + 1;
    uint16_t u16i;

    pu8TxBuffer[0] = 0xA5;
    pu8TxBuffer[1] = FptsMsg->u8ID;
    pu8TxBuffer[2] = u16FrmLength >> 8;
    pu8TxBuffer[3] = u16FrmLength;

    u8Checksum = pu8TxBuffer[1] + pu8TxBuffer[2] + pu8TxBuffer[3];

    for (u16i = 0; u16i < FptsMsg->u16Length; u16i++)
    {
        pu8TxBuffer[4 + u16i] = FptsMsg->pu8Payload[u16i];
        u8Checksum += FptsMsg->pu8Payload[u16i];
    }

    u8Checksum = ~u8Checksum;
    pu8TxBuffer[4 + u16i] = u8Checksum;

    RS232_SendBuf(siComPortNumber, pu8TxBuffer, u16FrmLength + 4);

    /* Flush sent message */
    FptsMsg->u8ID = 0;
    FptsMsg->u16Length = 0;
    for (u16i = 0; u16i < MAX_FRAME_LENGTH; u16i++)
    {
        FptsMsg->pu8Payload[u16i] = 0;
    }

    /* Blocking wait */
    sleep(Fu16Timeout);
    u16ByteCnt = RS232_PollComport(siComPortNumber, pu8RxBuffer, MAX_FRAME_LENGTH);

    /* Receive response frame */
    if ((pu8RxBuffer[0] == 0x5A) &&
                                    (u16ByteCnt > 4) &&
                                    ((pu8RxBuffer[1] & 0x0F) == FptsMsg->u8ID))
    {
        pu8Tmp = pu8RxBuffer;
        pu8Tmp ++; /* escape SOF */
        u16FrmLength = (((uint16_t)pu8RxBuffer[2] << 8) & 0xFF00) | (uint16_t)pu8RxBuffer[3];
        if ((u16FrmLength + 4) == u16ByteCnt)
        {
            u8Checksum = 0;
            for (u16i = 0; u16i < u16FrmLength + 3; u16i++)
            {
                u8Checksum += *pu8Tmp + u16i;
            }

            if (u8Checksum == 0)
            {
                u16FrmLength--;
                FptsMsg->u8ID = *pu8Tmp;
                FptsMsg->u16Length = u16FrmLength;
                pu8Tmp += 3;
                for (u16i = 0; u16i < u16FrmLength; u16i++)
                {
                    FptsMsg->pu8Response[u16i] = *(pu8Tmp + u16i);
                }
            } /* if (u8Checksum == 0) */
            else
            {
                bErrCheck = false;
            }
        } /* if ((u16FrmLength + 4) == iByteCnt) */
        else
        {
            bErrCheck = false;
        }
    } /* if ((pu8RxBuffer[0] == 0x5A) && (iByteCnt > 4) && ((pu8RxBuffer[1] & 0x0F) == FptsMsg->u8ID)) */
    else
    {
        bErrCheck = false;
    }
    bRetVal = bErrCheck;

    return bRetVal;
}
