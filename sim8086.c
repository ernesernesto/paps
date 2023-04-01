#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

//printf("data %c%c%c%c%c%c%c%c\n", BYTE_TO_BINARY(data));

u32 offset = 0;

typedef enum RegisterCode
{
    RegisterCode_None,

    AL,
    CL,
    DL,
    BL,
    AH,
    CH,
    DH,
    BH,

    AX,
    CX,
    DX,
    BX,
    SP,
    BP,
    SI,
    DI,

    BX_SI,
    BX_DI,
    BP_SI, 
    BP_DI, 

    RegisterCode_Count,
} RegisterCode;

typedef enum InstructionCode
{
    None, 

    Mov,
    Add,
    Or,
    Adc,
    Sbb,
    And,
    Sub,
    Xor,
    Cmp,

    Jo,
    Jno,
    Jb,
    Jnb,
    Je,
    Jne,
    Jbe,
    Jnbe,
    Js,
    Jns,
    Jp,
    Jnp,
    Jl,
    Jnl,
    Jle,
    Jnle,
    Loopne,
    Loope,
    Loop,
    Jcxz,

    InstructionCode_Count,
} InstructionCode;


typedef enum OperandCode
{
    Register,
    Memory,
    Immediate,

    OperandCode_Count,
} OperandCode;

typedef struct Operand
{
    OperandCode code;
    RegisterCode regCode;
    u8 size;
    s16 displacement;
    bool accumulator;

    char* literals;
} Operand;

typedef struct Instruction
{
    InstructionCode code;
    Operand operands[2];
} Instruction;

RegisterCode romTable[8] = 
{
    BX_SI, BX_DI, BP_SI, BP_DI, SI, DI, BP, BX
};

RegisterCode regTable8[8] = 
{
    AL, CL, DL, BL, AH, CH, DH, BH,
};

RegisterCode regTable16[8] = 
{
    AX, CX, DX, BX, SP, BP, SI, DI
};

RegisterCode* regTable[2] = { regTable8, regTable16 };

char* GetRegCodeStr(RegisterCode code)
{
    char* result = 0;
    switch(code)
    {
    case AL: { result = "al"; } break;
    case CL: { result = "cl"; } break;
    case DL: { result = "dl"; } break;
    case BL: { result = "bl"; } break;
    case AH: { result = "ah"; } break;
    case CH: { result = "ch"; } break;
    case DH: { result = "dh"; } break;
    case BH: { result = "bh"; } break;

    case AX: { result = "ax"; } break;
    case CX: { result = "cx"; } break;
    case DX: { result = "dx"; } break;
    case BX: { result = "bx"; } break;
    case SP: { result = "sp"; } break;
    case BP: { result = "bp"; } break;
    case SI: { result = "si"; } break;
    case DI: { result = "di"; } break;

    case BX_SI: { result = "bx + si"; } break;
    case BX_DI: { result = "bx + di"; } break;
    case BP_SI: { result = "bp + si"; } break;
    case BP_DI: { result = "bp + di"; } break;
    default: break;
    }

    return result;
}

char* GetInstructionCodeStr(InstructionCode code)
{
    char* result = 0;
    switch(code)
    {
    case Mov: { result = "mov"; } break;
    case Add: { result = "add"; } break;
    case Or: { result = "or"; } break;
    case Adc: { result = "adc"; } break;
    case Sbb: { result = "sbb"; } break;
    case And: { result = "and"; } break;
    case Sub: { result = "sub"; } break;
    case Xor: { result = "xor"; } break;
    case Cmp: { result = "cmp"; } break;

    case Jo:  { result = "jo"; } break;
    case Jno: { result = "jno"; } break;
    case Jb: { result = "jb"; } break;
    case Jnb: { result = "jnb"; } break;
    case Je: { result = "je"; } break;
    case Jne: { result = "jne"; } break;
    case Jbe: { result = "jbe"; } break;
    case Jnbe: { result = "jnbe"; } break;
    case Js: { result = "js"; } break;
    case Jns: { result = "jns"; } break;
    case Jp: { result = "jp"; } break;
    case Jnp: { result = "jnp"; } break;
    case Jl: { result = "jl"; } break;
    case Jnl: { result = "jnl"; } break;
    case Jle: { result = "jle"; } break;
    case Jnle: { result = "jnle"; } break;
    case Loopne: { result = "loopne"; } break;
    case Loope: { result = "loope"; } break;
    case Loop: { result = "loop"; } break;
    case Jcxz: { result = "jcxz"; } break;

    default: break;
    }

    return result;
}

Instruction RegRom(InstructionCode code, u8 byte1, u8 byte2, u8* buffer, u8 wide, u8 reg, u8 imm)
{
    Instruction result = {};
    result.code = code;

    u8 dir = (byte1 >> 1) & 0b1;
    u8 mod = (byte2 >> 6) & 0b11;
    u8 rom = (byte2 >> 0) & 0b111;

    char* op = GetInstructionCodeStr(code);

    if(imm == 0b1)
    {
        s16 data = byte2;

        if(wide)
        {
            u8 byte3 = buffer[offset++];
            data = (byte3 << 8) | byte2;
        }

        result.operands[0].code = Register;
        result.operands[0].regCode = regTable[wide][reg];
        result.operands[1].code = Immediate;
        result.operands[1].displacement = data;
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

                result.operands[0].code = Register;
                result.operands[0].regCode = regTable[wide][reg];
                result.operands[1].code = Memory;
                result.operands[1].displacement = data;
            }
            else
            {   
                if(dir)
                {
                    result.operands[0].code = Register;
                    result.operands[0].regCode = regTable[wide][reg];
                    result.operands[1].code = Memory;
                    result.operands[1].regCode = romTable[rom];
                }
                else
                {
                    result.operands[0].code = Memory;
                    result.operands[0].regCode = romTable[rom];
                    result.operands[1].code = Register;
                    result.operands[1].regCode = regTable[wide][reg];
                }
            }
        }
        else if(mod == 0b01)
        {
            s8 data = buffer[offset++];

            if(dir)
            {
                result.operands[0].code = Register;
                result.operands[0].regCode = regTable[wide][reg];
                result.operands[1].code = Memory;
                result.operands[1].regCode = romTable[rom];
                result.operands[1].displacement = data;
            }
            else
            {
                result.operands[0].code = Memory;
                result.operands[0].regCode = romTable[rom];
                result.operands[0].displacement = data;
                result.operands[1].code = Register;
                result.operands[1].regCode = regTable[wide][reg];
            }
        }
        else if(mod == 0b10)
        {
            u8 byte3 = buffer[offset++];
            u8 byte4 = buffer[offset++];
            s16 data = (byte4 << 8) | byte3;

            if(dir)
            {
                result.operands[0].code = Register;
                result.operands[0].regCode = regTable[wide][reg];
                result.operands[1].code = Memory;
                result.operands[1].regCode = romTable[rom];
                result.operands[1].displacement = data;
            }
            else
            {
                result.operands[0].code = Memory;
                result.operands[0].regCode = romTable[rom];
                result.operands[0].displacement = data;
                result.operands[1].code = Register;
                result.operands[1].regCode = regTable[wide][reg];
            }
        }
        else if(mod == 0b11)
        {
            u8 src = dir ? reg : rom;
            u8 dest = dir ? rom : reg;

            result.operands[0].code = Register;
            result.operands[0].regCode = regTable[wide][src];
            result.operands[1].code = Register;
            result.operands[1].regCode = regTable[wide][dest];
        }
    }

    return result;
}

Instruction AddOrAdcSbbAndSubXorCmpMov(InstructionCode code, u8 byte1, u8 byte2, u8* buffer)
{
    Instruction result = {};
    result.code = code;

    u8 dir = (byte1 >> 1) & 0b1;
    u8 wide = (byte1 >> 0) & 0b1;
    u8 mod = (byte2 >> 6) & 0b11;
    u8 rom = (byte2 >> 0) & 0b111;

    bool mov = code == Mov;
    char* literals = wide ? "word" : "byte";
    char* op = GetInstructionCodeStr(code);

    if(mod == 0b00)
    {
        if(rom == 0b110)
        {
            u8 byte3 = buffer[offset++];

            if(!dir && wide)
            {
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;

                result.operands[0].code = Immediate;
                result.operands[0].literals = literals;
                result.operands[1].displacement = data;
                result.operands[1].code = Memory;
                result.operands[1].regCode = romTable[rom];
            }
            else if(dir && !wide)
            {
                s16 data = byte3;

                result.operands[0].code = Memory;
                result.operands[0].regCode = romTable[rom];
                result.operands[0].literals = literals;
                result.operands[1].code = Immediate;
                result.operands[1].displacement = data;
            }
            else
            {
                u8 byte4 = buffer[offset++];
                u8 byte5 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;

                result.operands[0].code = Immediate;
                result.operands[0].literals = literals;
                result.operands[0].displacement = data;
                result.operands[1].code = Immediate;
                result.operands[1].displacement = byte5;
            }
        }
        else
        {   
            u8 byte3 = buffer[offset++];

            if(!dir && wide)
            {
                u8 byte4 = buffer[offset++];
                s16 data = (byte4 << 8) | byte3;
                if(mov)
                {
                    result.operands[0].code = Memory;
                    result.operands[0].regCode = romTable[rom];
                    result.operands[1].code = Immediate;
                    result.operands[1].size = 1;
                    result.operands[1].literals = "word";
                    result.operands[1].displacement = data;
                }
                else
                {
                    result.operands[0].code = Memory;
                    result.operands[0].regCode = romTable[rom];
                    result.operands[0].literals = "word";
                    result.operands[1].code = Immediate;
                    result.operands[1].displacement = data;
                }
            }
            else
            {
                s16 data = byte3;

                if(mov)
                {
                    result.operands[0].code = Memory;
                    result.operands[0].regCode = romTable[rom];
                    result.operands[1].code = Immediate;
                    result.operands[1].size = wide == 0 ? 0 : 1;
                    result.operands[1].literals = literals;
                    result.operands[1].displacement = data;
                }
                else
                {
                    result.operands[0].code = Memory;
                    result.operands[0].regCode = romTable[rom];
                    result.operands[0].literals = literals;
                    result.operands[1].code = Immediate;
                    result.operands[1].displacement = data;
                }
            }
        }
    }
    else if(mod == 0b01)
    {
        if(mov)
        {
            s8 displacement = buffer[offset++];
            s8 data = buffer[offset++];

            result.operands[0].code = Memory;
            result.operands[0].regCode = romTable[rom];
            result.operands[0].displacement = displacement;
            result.operands[1].code = Immediate;
            result.operands[1].size = wide == 0 ? 0 : 1;
            result.operands[1].literals = literals;
            result.operands[1].displacement = data;
        }
        else
        {
            u8 byte3 = buffer[offset++];
            u8 byte4 = buffer[offset++];
            u8 byte5 = buffer[offset++];
            s16 displacement = (byte4 << 8) | byte3;

            s16 data = byte5;
            if(wide)
            {
                u8 byte6 = buffer[offset++];
                data = (byte6 << 8) | byte5;
            }

            result.operands[0].code = Memory;
            result.operands[0].regCode = romTable[rom];
            result.operands[0].displacement = displacement;
            result.operands[0].literals = literals;
            result.operands[1].code = Immediate;
            result.operands[1].displacement = data;
        }

    }
    else if(mod == 0b10)
    {
        u8 byte3 = buffer[offset++];
        u8 byte4 = buffer[offset++];
        u8 byte5 = buffer[offset++];

        s16 displacement = (byte4 << 8) | byte3;
        s16 data = byte5;

        if(mov)
        {
            if(wide)
            {
                u8 byte6 = buffer[offset++];
                data = (byte6 << 8) | byte5;
            }

            result.operands[0].code = Memory;
            result.operands[0].regCode = romTable[rom];
            result.operands[0].displacement = displacement;
            result.operands[1].code = Immediate;
            result.operands[1].size = wide == 0 ? 0 : 1;
            result.operands[1].literals = literals;
            result.operands[1].displacement = data;
        }
        else
        {
            result.operands[0].code = Memory;
            result.operands[0].regCode = romTable[rom];
            result.operands[0].size = wide == 0 ? 0 : 1;
            result.operands[0].literals = literals;
            result.operands[0].displacement = displacement;
            result.operands[1].code = Immediate;
            result.operands[1].displacement = data;
        }

    }
    else if(mod == 0b11)
    {
        u8 byte3 = buffer[offset++];
        s16 data = byte3;

        if(!dir && wide)
        {
            u8 byte4 = buffer[offset++];
            data = (byte4 << 8) | byte3;
        }

        result.operands[0].code = Memory;
        result.operands[0].regCode = regTable[wide][rom];
        result.operands[1].code = Immediate;
        result.operands[1].displacement = data;
    }

    return result;
}
  
int main(int argc, char **argv) 
{
    if(argc == 0)
    {
        printf("No input file specified\n");
        return 0;
    }

    char* targetFile = 0;
    bool executionMode = false;
    if(argc == 3)
    {
        char* mode = argv[1];
        if(strcmp(mode, "-exec") == 0)
        {
            executionMode = true;
            targetFile = argv[2];
        }
        else
        {
            printf("Unknown command %s\n", mode);
            return 0;
        }
    }
    else
    {
        targetFile = argv[1];
    }

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

    if(executionMode)
    {
    }
    else
    {
        fprintf(stdout, "bits 16\n\n");
    }
        
    while(offset < fileSize)
    {
        Instruction instruction = {};

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
            u8 wide = (byte1 >> 0) & 0b1;
            u8 reg = (byte2 >> 3) & 0b111;
            u8 imm = (byte1 >> 2) & 0b1;
            if(imm == 0b1)
            {
                reg = 0;
            }
            instruction = RegRom(Add, byte1, byte2, buffer, wide, reg, imm);
        } break;

        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2c:
        case 0x2d:
        {
            u8 wide = (byte1 >> 0) & 0b1;
            u8 reg = (byte2 >> 3) & 0b111;
            u8 imm = (byte1 >> 2) & 0b1;
            if(imm == 0b1)
            {
                reg = 0;
            }
            instruction = RegRom(Sub, byte1, byte2, buffer, wide, reg, imm);
        } break;

        case 0x38:
        case 0x39:
        case 0x3a:
        case 0x3b:
        case 0x3c:
        case 0x3d:
        {
            u8 wide = (byte1 >> 0) & 0b1;
            u8 reg = (byte2 >> 3) & 0b111;
            u8 imm = (byte1 >> 2) & 0b1;
            if(imm == 0b1)
            {
                reg = 0;
            }
            instruction = RegRom(Cmp, byte1, byte2, buffer, wide, reg, imm);
        } break;

        case 0x70:
        {
            s8 ip = byte2;
            instruction.code = Jo;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x71:
        {
            s8 ip = byte2;
            instruction.code = Jno;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x72:
        {
            s8 ip = byte2;
            instruction.code = Jb;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x73:
        {
            s8 ip = byte2;
            instruction.code = Jnb;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x74:
        {
            s8 ip = byte2;
            instruction.code = Je;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x75:
        {
            s8 ip = byte2;
            instruction.code = Jne;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x76:
        {
            s8 ip = byte2;
            instruction.code = Jbe;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x77:
        {
            s8 ip = byte2;
            instruction.code = Jnbe;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x78:
        {
            s8 ip = byte2;
            instruction.code = Js;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x79:
        {
            s8 ip = byte2;
            instruction.code = Jns;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x7a:
        {
            s8 ip = byte2;
            instruction.code = Jp;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x7b:
        {
            s8 ip = byte2;
            instruction.code = Jnp;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x7c:
        {
            s8 ip = byte2;
            instruction.code = Jl;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x7d:
        {
            s8 ip = byte2;
            instruction.code = Jnl;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x7e:
        {
            s8 ip = byte2;
            instruction.code = Jle;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x7f:
        {
            s8 ip = byte2;
            instruction.code = Jnle;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        {
            InstructionCode instructionCodes[8] = { Add, Or, Adc, Sbb, And, Sub, Xor, Cmp };
            u8 instructionCodeIndex = (byte2 >> 3) & 0b111;
            InstructionCode instructionCode = instructionCodes[instructionCodeIndex];


            instruction = AddOrAdcSbbAndSubXorCmpMov(instructionCode, byte1, byte2, buffer);
        }
        break;

        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8b:
        case 0x8c:
        {
            u8 wide = (byte1 >> 0) & 0b1;
            u8 reg = (byte2 >> 3) & 0b111;
            u8 imm = 0;
            instruction = RegRom(Mov, byte1, byte2, buffer, wide, reg, imm);
        } break;

        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        {
            char* op = GetInstructionCodeStr(Mov);
            u8 dir = (byte1 >> 1) & 0b1;
            u8 wide = (byte1 >> 0) & 0b1;
            u8 reg = 0;

            s16 data = byte2;

            if(wide)
            {
                u8 byte3 = buffer[offset++];
                data = (byte3 << 8) | byte2;
            }
            
            instruction.code = Mov;
                                          
            if(dir)
            {
                instruction.operands[0].code = Immediate;
                instruction.operands[0].displacement = data;
                instruction.operands[0].accumulator = true;
                instruction.operands[1].code = Register;
                instruction.operands[1].regCode = regTable[wide][reg];
            }
            else
            {
                instruction.operands[0].code = Register;
                instruction.operands[0].regCode = regTable[wide][reg];
                instruction.operands[1].code = Immediate;
                instruction.operands[1].displacement = data;
                instruction.operands[1].accumulator = true;
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
            u8 imm = 1;
            instruction = RegRom(Mov, byte1, byte2, buffer, wide, reg, imm);
        } break;

        case 0xc6:
        case 0xc7:
        {
            instruction = AddOrAdcSbbAndSubXorCmpMov(Mov, byte1, byte2, buffer);
        } break;

        case 0xe0:
        {
            s8 ip = byte2;
            instruction.code = Loopne;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0xe1:
        {
            s8 ip = byte2;
            instruction.code = Loope;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0xe2:
        {
            s8 ip = byte2;
            instruction.code = Loop;
            instruction.operands[0].displacement = ip + 2;
        } break;

        case 0xe3:
        {
            s8 ip = byte2;
            instruction.code = Jcxz;
            instruction.operands[0].displacement = ip + 2;
        } break;


        default: 
        {
            printf("0x%x unimplemented\n", byte1);
        } break;

        }

        if(executionMode)
        {
        }
        else
        {
            char* instructionCode = GetInstructionCodeStr(instruction.code);

            Operand leftOperand = instruction.operands[0];
            Operand rightOperand = instruction.operands[1];

            char* leftOperandStr = GetRegCodeStr(leftOperand.regCode);
            char* rightOperandStr = GetRegCodeStr(rightOperand.regCode);

            fprintf(stdout, "%s ", instructionCode);

            switch(instruction.code)
            {

            case Add:
            case Or:
            case Adc:
            case Sbb:
            case And:
            case Sub:
            case Xor:
            case Cmp:
            {
                if(leftOperand.code == Register)
                {
                    fprintf(stdout, "%s, ", leftOperandStr);
                }
                else if(leftOperand.code == Memory)
                {
                    if(rightOperand.code == Immediate)
                    {
                        if(leftOperand.literals)
                        {
                            fprintf(stdout, "%s [%s", leftOperand.literals, leftOperandStr);

                            if(leftOperand.displacement)
                            {
                                fprintf(stdout, " + %d", leftOperand.displacement);
                            }

                            fprintf(stdout, "], ");
                        }
                        else
                        {
                            fprintf(stdout, "%s, ", leftOperandStr);
                        }
                    }
                    else
                    {
                        fprintf(stdout, "[%s", leftOperandStr);

                        if(leftOperand.displacement)
                        {
                            fprintf(stdout, " + %d", leftOperand.displacement);
                        }

                        fprintf(stdout, "], ");
                    }
                }

                if(rightOperand.code == Register)
                {
                    fprintf(stdout, "%s", rightOperandStr);
                }
                else if(rightOperand.code == Memory)
                {
                    fprintf(stdout, "[");
                    if(rightOperand.regCode != RegisterCode_None)
                    {
                        char* rightOperandStr = GetRegCodeStr(rightOperand.regCode);
                        fprintf(stdout, "%s", rightOperandStr);

                        if(rightOperand.displacement)
                        {
                            fprintf(stdout, " + %d", rightOperand.displacement);
                        }
                    }
                    else
                    {
                        fprintf(stdout, "%d", rightOperand.displacement);
                    }

                    fprintf(stdout, "]");
                }
                else if(rightOperand.code == Immediate)
                {
                    fprintf(stdout, "%d", rightOperand.displacement);
                }

                fprintf(stdout, "\n");
            } break;

            case Mov: 
            {
                if(leftOperand.code == Register)
                {
                    fprintf(stdout, "%s, ", leftOperandStr);
                }
                else if(leftOperand.code == Memory)
                {
                    fprintf(stdout, "[%s", leftOperandStr);

                    if(leftOperand.displacement)
                    {
                        fprintf(stdout, " + %d", leftOperand.displacement);
                    }

                    fprintf(stdout, "], ");
                }
                else if(leftOperand.code == Immediate)
                {
                    if(leftOperand.accumulator)
                    {
                        fprintf(stdout, "[%d], ", leftOperand.displacement);
                    }
                    else
                    {
                        fprintf(stdout, "%d, ", leftOperand.displacement);
                    }
                }

                if(rightOperand.code == Register)
                {
                    fprintf(stdout, "%s", rightOperandStr);
                }
                else if(rightOperand.code == Memory)
                {
                    fprintf(stdout, "[");
                    if(rightOperand.regCode != RegisterCode_None)
                    {
                        fprintf(stdout, "%s", rightOperandStr);

                        if(rightOperand.displacement)
                        {
                            fprintf(stdout, " + %d", rightOperand.displacement);
                        }
                    }
                    else
                    {
                        fprintf(stdout, "%d", rightOperand.displacement);
                    }

                    fprintf(stdout, "]");
                }
                else if(rightOperand.code == Immediate)
                {
                    if(rightOperand.accumulator)
                    {
                        fprintf(stdout, "[%d]", rightOperand.displacement);
                    }
                    else
                    {
                        if(rightOperand.literals)
                        {
                            fprintf(stdout, "%s ", rightOperand.literals);
                        }
                        fprintf(stdout, "%d", rightOperand.displacement);
                    }
                }

                fprintf(stdout, "\n");

            } break;

            case Jo:
            case Jno:
            case Jb: 
            case Jnb: 
            case Je: 
            case Jne: 
            case Jbe: 
            case Jnbe:
            case Js: 
            case Jns:
            case Jp: 
            case Jnp:
            case Jl: 
            case Jnl:
            case Jle:
            case Jnle: 
            case Loopne:
            case Loope:
            case Loop:
            case Jcxz:
            { 
                fprintf(stdout, "$+%d\n", instruction.operands[0].displacement);
            }
            break;


            default: break;
            }
        }
    }

    return 0;
}
