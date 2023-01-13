#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#define LOG_GROUP_BYTE_PER_LINE (32)

extern FILE * LogStream;

void Logger_Init(void);
void Logger_Append(char* FpcString);
void Logger_Close(void);
void Logger_AppendArray(char *FpcText, uint8_t *Fpu8Array, uint16_t Fu16Len);

#endif // LOGGER_H_INCLUDED
