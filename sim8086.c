#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define u8 uint8_t 
#define u16 uint16_t 
#define u32 uint32_t 
#define i32 int32_t 

#define BYTE_TO_BINARY(val)  \
  (val & 0x80 ? '1' : '0'), \
  (val & 0x40 ? '1' : '0'), \
  (val & 0x20 ? '1' : '0'), \
  (val & 0x10 ? '1' : '0'), \
  (val & 0x08 ? '1' : '0'), \
  (val & 0x04 ? '1' : '0'), \
  (val & 0x02 ? '1' : '0'), \
  (val & 0x01 ? '1' : '0') 

char* registers[16] = 
{
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};

char* rmEncodings[8] = 
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
        char buffer[256];
        sprintf(buffer, "Cannot open file %s\n", targetFile);
        printf("%s", buffer);
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

        u8 opCode = (byte1 >> 4);

        //printf("byte1 %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(byte1));
        //printf("byte2 %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(byte2));
        
        if(opCode == 0b1000) // Register/memory to/from register\n
        {
            // D is set, REG field source, otherwise dest
            // If W is set, word operations otherwise byte operations
            u8 d = (byte1 >> 1) & 0b1;
            u8 w = (byte1 >> 0) & 0b1;
            u8 mod = (byte2 >> 6) & 0b11;
            u8 reg = (byte2 >> 3) & 0b111;
            u8 rm = (byte2 >> 0) & 0b111;

            if(w)
            {
                reg |= 0b1000;
            }

            if(mod == 0b00)
            {
                if(rm == 0b110)
                {
                }
                else
                {   
                    if(d)
                    {
                        printf("mov %s, [%s]\n", registers[reg], rmEncodings[rm]);
                    }
                    else
                    {
                        printf("mov [%s], %s\n", rmEncodings[rm], registers[reg]);
                    }
                }
            }
            else if(mod == 0b01)
            {
                u8 byte3 = buffer[offset++];
                if(d)
                {
                    printf("mov %s, [%s + %d]\n", registers[reg], rmEncodings[rm], byte3);
                }
                else
                {
                    printf("mov [%s + %d], %s\n", rmEncodings[rm], byte3, registers[reg]);
                }
            }
            else if(mod == 0b10)
            {
                u8 byte3 = buffer[offset++];
                u8 byte4 = buffer[offset++];
                u16 data = byte3 | (byte4 << 8);
                printf("mov %s, [%s + %d]\n", registers[reg], rmEncodings[rm], data);
            }
            else if(mod == 0b11)
            {
                if(w)
                {
                    rm |= 0b1000;
                }

                u8 src = d ? reg : rm;
                u8 dest = d ? rm : reg;

                printf("mov %s, %s\n", registers[src], registers[dest]);
            }
        }
        else if(opCode == 0b1100)
        {
            printf("Immediate register to memory\n");
        }
        else if(opCode == 0b1011) // Immediate to register 
        {
            u8 w = (byte1 >> 3) & 0b1;
            u8 reg = (byte1 >> 0) & 0b111;

            if(w)
            {
                reg |= 0b1000;
                u8 byte3 = buffer[offset++];
                u16 data = byte2 | (byte3 << 8);

                printf("mov %s, %d\n", registers[reg], data);
            }
            else
            {
                printf("mov %s, %d\n", registers[reg], byte2);
            }
        }
        else if(opCode == 0b1010)
        {
            printf("Memory to accumulator | accumulator to memory \n");
        }
        else if(opCode == 0b1010)
        {
            printf("Register/memory to segment register | Segment register to register/memory");
        }
    }

    return 0;
}
