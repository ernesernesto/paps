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

#if 0
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define LOG(fmt, ...) 
#endif

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

u32 offset = 0;
void RomRom(char* op, u8 byte1, u8 byte2, u8* buffer)
{
    u8 dir = (byte1 >> 1) & 0b1;
    u8 wide = (byte1 >> 0) & 0b1;
    u8 mod = (byte2 >> 6) & 0b11;
    u8 reg = (byte2 >> 3) & 0b111;
    u8 rom = (byte2 >> 0) & 0b111;
    u8 imm = (byte1 >> 2) & 0b1;

    if(imm == 0b1)
    {
        s16 data = byte2;

        if(wide)
        {
            u8 byte3 = buffer[offset++];
            data = (byte3 << 8) | byte2;
        }

        printf("%s %s, %d\n", op, regTable[wide][0], data);
    }
    else
    {
        if(mod == 0b00)
        {
            if(rom == 0b110)
            {
                u8 byte3 = buffer[offset++];
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;
                printf("%s %s, [%d]\n", op, regTable[wide][reg], data);
            }
            else
            {   
                if(dir)
                {
                    printf("%s %s, [%s]\n", op, regTable[wide][reg], romTable[rom]);
                }
                else
                {
                    printf("%s [%s], %s\n", op, romTable[rom], regTable[wide][reg]);
                }
            }
        }
        else if(mod == 0b01)
        {
            s8 data = buffer[offset++];

            if(dir)
            {
                printf("%s %s, [%s + %d]\n", op, regTable[wide][reg], romTable[rom], data);
            }
            else
            {
                printf("%s [%s + %d], %s\n", op, romTable[rom], data, regTable[wide][reg]);
            }
        }
        else if(mod == 0b10)
        {
            u8 byte3 = buffer[offset++];
            u8 byte4 = buffer[offset++];
            s16 data = (byte4 << 8) | byte3;

            if(dir)
            {
                printf("%s %s, [%s + %d]\n", op, regTable[wide][reg], romTable[rom], data);
            }
            else
            {
                printf("%s [%s + %d], %s\n", op, romTable[rom], data, regTable[wide][reg]);
            }
        }
        else if(mod == 0b11)
        {
            u8 src = dir ? reg : rom;
            u8 dest = dir ? rom : reg;

            printf("%s %s, %s\n", op, regTable[wide][src], regTable[wide][dest]);
        }
    }
}

void AddOrAdcSbbAndSubXorCmp(u8 byte1, u8 byte2, u8* buffer)
{
    char* opcodes[8] = { "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp" };
    u8 opcodeIndex = (byte2 >> 3) & 0b111;
    char* op = opcodes[opcodeIndex];

    u8 dir = (byte1 >> 1) & 0b1;
    u8 wide = (byte1 >> 0) & 0b1;
    u8 mod = (byte2 >> 6) & 0b11;
    u8 rom = (byte2 >> 0) & 0b111;

    char* size = wide ? "word" : "byte";

    if(mod == 0b00)
    {
        if(rom == 0b110)
        {
            u8 byte3 = buffer[offset++];

            u8 check = (byte1 >> 0) & 0b11;
            if(check == 0b01)
            {
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;
                printf("%s %s [%d], %s\n", op, size, data, romTable[rom]);
            }
            else if(check == 0b10)
            {
                s16 data = byte3;
                printf("%s %s [%s], %d\n", op, size, romTable[rom], data);
            }
            else
            {
                u8 byte4 = buffer[offset++];
                u8 byte5 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;
                printf("%s %s [%d], %d\n", op, size, data, byte5);
            }
        }
        else
        {   
            u8 byte3 = buffer[offset++];

            u8 check = (byte1 >> 0) & 0b11;
            if(check == 0b01)
            {
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;
                printf("%s %s [%s], %d\n", op, size, romTable[rom], data);
            }
            else
            {
                s16 data = byte3;
                printf("%s %s [%s], %d\n", op, size, romTable[rom], data);
            }
        }
    }
    else if(mod == 0b01)
    {
        u8 byte3 = buffer[offset++];
        u8 byte4 = buffer[offset++];
        u8 byte5 = buffer[offset++];
        s16 displacement = (byte4 << 8) | byte3;

        s16 data = byte5;
        u8 check = (byte1 >> 0) & 0b11;
        if(check == 0b01)
        {
            u8 byte6 = buffer[offset++];
            data = (byte6 << 8) | byte5;
        }

        printf("%s %s [%s + %d], %d\n", op, size, romTable[rom], displacement, data);
    }
    else if(mod == 0b10)
    {
        u8 byte3 = buffer[offset++];
        u8 byte4 = buffer[offset++];
        u8 byte5 = buffer[offset++];
        s16 displacement = (byte4 << 8) | byte3;

        s16 data = byte5;
        u8 check = (byte1 >> 0) & 0b11;
        if(check == 0b01)
        {
            u8 byte6 = buffer[offset++];
            data = (byte6 << 8) | byte5;
        }

        printf("%s %s [%s + %d], %d\n", op, size, romTable[rom], displacement, data);
    }
    else if(mod == 0b11)
    {
        u8 byte3 = buffer[offset++];
        s16 data = byte3;

        u8 check = (byte1 >> 0) & 0b11;
        if(check == 0b01)
        {
            u8 byte4 = buffer[offset++];
            data = (byte4 << 8) | byte3;
        }

        printf("%s %s, %d\n", op, regTable[wide][rom], data);
    }
}
  
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
    while(offset < fileSize)
    {
        u8 byte1 = buffer[offset++];
        u8 byte2 = buffer[offset++];

        LOG("0x%x\n", byte1);
        LOG("byte1 %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(byte1));
        LOG("byte2 %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(byte2));

        // Note: https://edge.edx.org/c4x/BITSPilani/EEE231/asset/8086_family_Users_Manual_1_.pdf
        // Intel manual 8086 guide -- Machine Instruction Decoding Guide
        //
        // Byte 1     | Byte 2      | Byte 3            | Byte 4            | Byte 5        | Byte 6
        // OPCODE_D_W | MOD_REG_RM  | LOD DISP / Data   | HI DISP / DATA    | LOW DATA      | HI DATA 
        // 000000_0_0 | 00_000_000
        //
        // MOD :
        // 00 Memory mode no displacement (When R/M 110 16 bit displacement follows)
        // 01 Memory mode 8 bit displacement
        // 10 Memory mode 16 bit displacement
        // 11 Register mode no displacement

        switch(byte1)
        {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        {
            RomRom("add", byte1, byte2, buffer);
        } break;

        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2d:
        {
            RomRom("sub", byte1, byte2, buffer);
        } break;

        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        {
            RomRom("cmp", byte1, byte2, buffer);
        } break;

        case 0x70:
        {
            s8 ip = byte2;
            printf("jo $+%d\n", ip + 2);
        } break;

        case 0x71:
        {
            s8 ip = byte2;
            printf("jno $+%d\n", ip + 2);
        } break;

        case 0x72:
        {
            s8 ip = byte2;
            printf("jb $+%d\n", ip + 2);
        } break;

        case 0x73:
        {
            s8 ip = byte2;
            printf("jnb $+%d\n", ip + 2);
        } break;

        case 0x74:
        {
            s8 ip = byte2;
            printf("je $+%d\n", ip + 2);
        } break;

        case 0x75:
        {
            s8 ip = byte2;
            printf("jne $+%d\n", ip + 2);
        } break;

        case 0x76:
        {
            s8 ip = byte2;
            printf("jbe $+%d\n", ip + 2);
        } break;

        case 0x77:
        {
            s8 ip = byte2;
            printf("jnbe $+%d\n", ip + 2);
        } break;

        case 0x78:
        {
            s8 ip = byte2;
            printf("js $+%d\n", ip + 2);
        } break;

        case 0x79:
        {
            s8 ip = byte2;
            printf("jns $+%d\n", ip + 2);
        } break;

        case 0x7a:
        {
            s8 ip = byte2;
            printf("jp $+%d\n", ip + 2);
        } break;

        case 0x7b:
        {
            s8 ip = byte2;
            printf("jnp $+%d\n", ip + 2);
        } break;

        case 0x7c:
        {
            s8 ip = byte2;
            printf("jl $+%d\n", ip + 2);
        } break;

        case 0x7d:
        {
            s8 ip = byte2;
            printf("jnl $+%d\n", ip + 2);
        } break;

        case 0x7e:
        {
            s8 ip = byte2;
            printf("jle $+%d\n", ip + 2);
        } break;

        case 0x7f:
        {
            s8 ip = byte2;
            printf("jnle $+%d\n", ip + 2);
        } break;

        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        {
            AddOrAdcSbbAndSubXorCmp(byte1, byte2, buffer);
        }
        break;

        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        {
            char* op = "mov";
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
                    printf("%s %s, [%d]\n", op, regTable[wide][reg], data);
                }
                else
                {   
                    if(dir)
                    {
                        printf("%s %s, [%s]\n", op, regTable[wide][reg], romTable[rom]);
                    }
                    else
                    {
                        printf("%s [%s], %s\n", op, romTable[rom], regTable[wide][reg]);
                    }
                }
            }
            else if(mod == 0b01)
            {
                s8 data = buffer[offset++];

                if(dir)
                {
                    printf("%s %s, [%s + %d]\n", op, regTable[wide][reg], romTable[rom], data);
                }
                else
                {
                    printf("%s [%s + %d], %s\n", op, romTable[rom], data, regTable[wide][reg]);
                }
            }
            else if(mod == 0b10)
            {
                u8 byte3 = buffer[offset++];
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;

                if(dir)
                {
                    printf("%s %s, [%s + %d]\n", op, regTable[wide][reg], romTable[rom], data);
                }
                else
                {
                    printf("%s [%s + %d], %s\n", op, romTable[rom], data, regTable[wide][reg]);
                }
            }
            else if(mod == 0b11)
            {
                u8 src = dir ? reg : rom;
                u8 dest = dir ? rom : reg;

                printf("%s %s, %s\n", op, regTable[wide][src], regTable[wide][dest]);
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
            char* op = "mov";
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
                        printf("%s [%s], %s %d\n", op, romTable[rom], size, data);
                    }
                    else
                    {
                        s8 data = buffer[offset++];
                        printf("%s [%s], %s %d\n", op, romTable[rom], size, data);
                    }
                }
            }
            else if(mod == 0b01)
            {
                s8 displacement = buffer[offset++];
                s8 data = buffer[offset++];
                printf("%s [%s + %d], %s %d\n", op, romTable[rom], displacement, size, data);
            }
            else if(mod == 0b10)
            {
                u8 byte3 = buffer[offset++];
                u8 byte4 = buffer[offset++];
                u8 byte5 = buffer[offset++];
                u8 byte6 = buffer[offset++];
                s16 displacement = (byte4 << 8) | byte3;
                s16 data = (byte6 << 8) | byte5;

                printf("%s [%s + %d], %s %d\n", op, romTable[rom], displacement, size, data);
            }
        } break;

        case 0xe0:
        {
            s8 ip = byte2;
            printf("loopne $+%d\n", ip + 2);
        } break;

        case 0xe1:
        {
            s8 ip = byte2;
            printf("loope $+%d\n", ip + 2);
        } break;

        case 0xe2:
        {
            s8 ip = byte2;
            printf("loop $+%d\n", ip + 2);
        } break;

        case 0xe3:
        {
            s8 ip = byte2;
            printf("jcxz $+%d\n", ip + 2);
        } break;


        default: 
        {
            printf("0x%x unimplemented\n", byte1);
        } break;

        }
    }

    return 0;
}
