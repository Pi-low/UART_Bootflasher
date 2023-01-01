#ifndef CORE_H_INCLUDED
#define CORE_H_INCLUDED

#include <stdint.h>

void Core_InitDataBlockGen(void);
bool Core_CbDataBlockGen(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
bool Core_CbFetchLogisticData(uint32_t Fu32addr, const uint8_t* Fpu8Buffer, uint8_t Fu8Size);
void Core_PreParse(uint8_t* Fpu8Buffer, uint32_t* Fpu32Len);
void Core_GetSwInfo(uint16_t* Fpu16Version, uint8_t* Fpu8Decription);

#endif // CORE_H_INCLUDED
