#ifndef CRC16_H
#define CRC16_H

void Crc16_KermitUpdate(uint16_t* Fpu16Input, uint8_t Fu8Data);
void Crc16_BufferUpdate(uint16_t* Fpu16Input, uint8_t* Fpu8Data, uint16_t Fu16Length);

#endif // CRC16_H
