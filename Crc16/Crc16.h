#ifndef CRC16_H
#define CRC16_H

void updateCrc16(uint16_t* Fpu16Input, uint8_t Fu8Data);
void BufUpdateCrc16(uint16_t* Fpu16Input, uint8_t* Fpu8Data, uint16_t Fu16Length);

#endif // CRC16_H
