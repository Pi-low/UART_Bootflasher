#ifndef COMPORT_H_INCLUDED
#define COMPORT_H_INCLUDED

#define MAX_COM_PORT_NB (48)
#define UNIT_TIME_DIV (50) //ms

#include <stdint.h>
#include <stdbool.h>
#include "../Bootloader/Bootloader.h"

void ComPort_Scan(void);
bool ComPort_Open(char* FpcString);
void ComPort_Close(void);
bool ComPort_SendGenericFrame(tsFrame* FptsSendMsg, uint16_t Fu16Timeout);

#endif // COMPORT_H_INCLUDED
