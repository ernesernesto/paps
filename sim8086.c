#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define u8  uint8_t 
#define u16 uint16_t 
#define u32 uint32_t 
#define s8  int8_t 
#define s16 int16_t 
#define s32 int32_t 

#define BYTE_TO_BINARY(val)  \
  (val & 0x80 ? '1' : '0'), \
  (val & 0x40 ? '1' : '0'), \
  (val & 0x20 ? '1' : '0'), \
  (val & 0x10 ? '1' : '0'), \
  (val & 0x08 ? '1' : '0'), \
  (val & 0x04 ? '1' : '0'), \
  (val & 0x02 ? '1' : '0'), \
  (val & 0x01 ? '1' : '0') 

char* regTable8[8] = 
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
};

char* regTable16[8] = 
{
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

char** regTable[2] = { regTable8, regTable16 };

char* romTable[8] = 
{
    "bx + si", "bx + di", "bp + si", "bp + di", "si", "di", "bp", "bx"
};

//printf("data %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(data));

int main(int argc, char **argv) 
{
    if(argc == 0)
    {
        printf("No input file specified\n");
        return 0;
    }

    char* targetFile = argv[1];

    FILE* file = fopen(targetFile, "r");
    if(!file)
    {
        printf("Cannot open file %s\n", targetFile);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    u32 fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8* buffer = malloc(fileSize);
    int count = fread(buffer, sizeof(char), fileSize, file);
    fclose(file);
        
    printf("bits 16\n\n");
    u32 offset = 0;
    while(offset < fileSize)
    {
        u8 byte1 = buffer[offset++];
        u8 byte2 = buffer[offset++];

        //printf("byte1 %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(byte1));
        //printf("byte2 %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(byte2));

        // Note: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
        // Intel manual 8086 guide -- Machine Instruction Decoding Guide
        switch(byte1)
        {
        case 0x85:
        {
        } break;

        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        {
            u8 dir = (byte1 >> 1) & 0b1;
            u8 wide = (byte1 >> 0) & 0b1;
            u8 mod = (byte2 >> 6) & 0b11;
            u8 reg = (byte2 >> 3) & 0b111;
            u8 rom = (byte2 >> 0) & 0b111;

            if(mod == 0b00)
            {
                if(rom == 0b110)
                {
                    u8 byte3 = buffer[offset++];
                    u8 byte4 = buffer[offset++];
                    s16 data = (byte4 << 8) | byte3;
                    printf("mov %s, [%d]\n", regTable[wide][reg], data);
                }
                else
                {   
                    if(dir)
                    {
                        printf("mov %s, [%s]\n", regTable[wide][reg], romTable[rom]);
                    }
                    else
                    {
                        printf("mov [%s], %s\n", romTable[rom], regTable[wide][reg]);
                    }
                }
            }
            else if(mod == 0b01)
            {
                s8 data = buffer[offset++];

                if(dir)
                {
                    printf("mov %s, [%s + %d]\n", regTable[wide][reg], romTable[rom], data);
                }
                else
                {
                    printf("mov [%s + %d], %s\n", romTable[rom], data, regTable[wide][reg]);
                }
            }
            else if(mod == 0b10)
            {
                u8 byte3 = buffer[offset++];
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;

                if(dir)
                {
                    printf("mov %s, [%s + %d]\n", regTable[wide][reg], romTable[rom], data);
                }
                else
                {
                    printf("mov [%s + %d], %s\n", romTable[rom], data, regTable[wide][reg]);
                }
            }
            else if(mod == 0b11)
            {
                u8 src = dir ? reg : rom;
                u8 dest = dir ? rom : reg;

                printf("mov %s, %s\n", regTable[wide][src], regTable[wide][dest]);
            }
        } break;

        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        {
            u8 dir = (byte1 >> 1) & 0b1;
            u8 wide = (byte1 >> 0) & 0b1;

            u8 byte3 = buffer[offset++];
            s16 data = (byte3 << 8) | byte2;

            if(dir)
            {
                printf("mov [%d], %s\n", data, regTable[wide][0]);
            }
            else
            {
                printf("mov %s, [%d]\n", regTable[wide][0], data);
            }
        } break;

        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
        {
            u8 wide = (byte1 >> 3) & 0b1;
            u8 reg = (byte1 >> 0) & 0b111;

            if(wide)
            {
                u8 byte3 = buffer[offset++];
                s16 data = (byte3 << 8) | byte2;

                printf("mov %s, %d\n", regTable[wide][reg], data);
            }
            else
            {
                printf("mov %s, %d\n", regTable[wide][reg], byte2);
            }
        } break;

        case 0xc6:
        case 0xc7:
        {
            u8 wide = (byte1 >> 0) & 0b1;
            u8 mod = (byte2 >> 6) & 0b11;
            u8 rom = (byte2 >> 0) & 0b111;

            char* size = wide ? "word" : "byte";

            if(mod == 0b00)
            {
                if(rom == 0b110)
                {
                }
                else
                {   
                    if(wide)
                    {
                        u8 byte3 = buffer[offset++];
                        u8 byte4 = buffer[offset++];
                        s16 data = (byte4 << 8) | byte3;
                        printf("mov [%s], word %d\n", romTable[rom], data);
                    }
                    else
                    {
                        s8 data = buffer[offset++];
                        printf("mov [%s], byte %d\n", romTable[rom], data);
                    }
                }
            }
            else if(mod == 0b01)
            {
                s8 displacement = buffer[offset++];
                s8 data = buffer[offset++];
                printf("mov [%s + %d], %s %d\n", romTable[rom], displacement, size, data);
            }
            else if(mod == 0b10)
            {
                u8 byte3 = buffer[offset++];
                u8 byte4 = buffer[offset++];
                u8 byte5 = buffer[offset++];
                u8 byte6 = buffer[offset++];
                s16 displacement = (byte4 << 8) | byte3;
                s16 data = (byte6 << 8) | byte5;

                printf("mov [%s + %d], %s %d\n", romTable[rom], displacement, size, data);
            }
        }
        break;

        default: 
        {
            printf("0x%x unimplemented\n", byte1);
        } break;

        }
    }

    return 0;
}
