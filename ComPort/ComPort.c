#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../RS-232/rs232.h"
#include "ComPort.h"

void ComPort_Scan(void)
{
    uint8_t u8Cnt = 0, u8ListCnt = 0;
    uint8_t pu8ListComPort[MAX_COM_PORT_NB];

    for (u8Cnt = 0; u8Cnt < MAX_COM_PORT_NB; u8Cnt++)
    {
        if (!RS232_OpenComport(u8Cnt, 115200,"8N1", 0))
        {
            pu8ListComPort[u8ListCnt] = u8Cnt;
            u8ListCnt++;
            RS232_CloseComport(u8Cnt);
        }
    }
    printf("Available COM ports:\r\n");
    for (u8Cnt = 0; u8Cnt < u8ListCnt; u8Cnt++)
    {
        printf("[%u] COM%u\r\n", u8Cnt + 1, pu8ListComPort[u8Cnt] + 1);
    }
}
