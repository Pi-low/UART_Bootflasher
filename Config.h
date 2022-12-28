#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/* CONFIGURATION */
#define ALLOCATION_SIZE 1024
#define BLOCK_SIZE 128

typedef struct
{
    uint8_t * pu8FileStream;
    uint32_t u32FileSize;
    uint32_t u32Cursor;
} TsMyFile;

#endif // CONFIG_H_INCLUDED
