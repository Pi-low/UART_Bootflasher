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

#endif // MAIN_H_INCLUDED
