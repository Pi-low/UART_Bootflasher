#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

typedef enum
{
    eStateSelectComPort,
    eStateSelectFile,
    eStateTargetInfo,
    eStateFlashTarget,
    eStateQuit
} teMainStates;

extern const char* pcBuildDateTime;
extern const uint8_t u8ToolVersion;

#endif // MAIN_H_INCLUDED
