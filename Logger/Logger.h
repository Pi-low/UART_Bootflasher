#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

extern FILE * LogStream;

void Logger_Init(void);
void Logger_Log(char* FpcString);
void Logger_Close(void);

#endif // LOGGER_H_INCLUDED
