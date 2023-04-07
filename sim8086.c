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

    IP,

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
    OperandCode opCode;
    RegisterCode regCode;
    s16 displacement;

    char* literals;
} Operand;

typedef struct Instruction
{
    InstructionCode instCode;
    Operand operands[2];
} Instruction;

typedef enum Flags
{
    FLAGS_C = 1 << 0,
    FLAGS_P = 1 << 1,
    FLAGS_A = 1 << 2,
    FLAGS_Z = 1 << 3,
    FLAGS_S = 1 << 4,
    FLAGS_T = 1 << 5,
    FLAGS_I = 1 << 6,
    FLAGS_D = 1 << 7,
    FLAGS_O = 1 << 8,
} Flags;

#define FLAGS_COUNT 9 

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

typedef struct Registers
{
    s16 ax;
    s16 cx;
    s16 dx;
    s16 bx;
    s16 sp;
    s16 bp;
    s16 si;
    s16 di;

    s16 ip;
} Registers;

static Registers regs = {};
static s16 ip = 0;
static s16 flags;

typedef struct HandleInstructionResult
{
    s16 regBefore;
    s16 regAfter;

    s16 prevFlags;
} HandleInstructionResult;


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

    case IP: { result = "ip"; } break;
    default: break;
    }

    return result;
}

s16 *GetRegister(Registers *registers, RegisterCode code)
{
    s16 *result = 0;
    switch(code)
    {
    case AX: { result = &registers->ax; } break;
    case CX: { result = &registers->cx; } break;
    case DX: { result = &registers->dx; } break;
    case BX: { result = &registers->bx; } break;
    case SP: { result = &registers->sp; } break;
    case BP: { result = &registers->bp; } break;
    case SI: { result = &registers->si; } break;
    case DI: { result = &registers->di; } break;
    case IP: { result = &registers->ip; } break;
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

char* GetFlagsStr(Flags flag)
{
    char* result = 0;
    switch(flag)
    {

    case FLAGS_C: { result = "C"; } break;
    case FLAGS_P: { result = "P"; } break;
    case FLAGS_A: { result = "A"; } break;
    case FLAGS_Z: { result = "Z"; } break;
    case FLAGS_S: { result = "S"; } break;
    case FLAGS_T: { result = "T"; } break;
    case FLAGS_I: { result = "I"; } break;
    case FLAGS_D: { result = "D"; } break;
    case FLAGS_O: { result = "O"; } break;

    default: break;
    }

    return result;
}

void UpdateFlags(int val)
{
    bool bitSet = flags & FLAGS_S;
    if(bitSet)
    {
        if(val >= 0)
        {
            flags ^= FLAGS_S;
        }
    }
    else
    {
        if(val < 0)
        {
            flags ^= FLAGS_S;
        }
    }

    bitSet = flags & FLAGS_Z;
    if(bitSet)
    {
        if(val != 0)
        {
            flags ^= FLAGS_Z;
        }
    }
    else
    {
        if(val == 0)
        {
            flags ^= FLAGS_Z;
        }
    }

    //int setCount = 0;
    //for(int index = 0; index < 16; ++index)
    //{
    //    if(flags & (1 << index))
    //    {
    //        ++setCount;
    //    }
    //}

    //bool even = false;
    //if(setCount > 1)
    //{
    //    even = (((int)(float)setCount / 2.0f) == 0);
    //}

    //printf("setcount %d even %d \n", setCount, even);
    //bitSet = flags & FLAGS_P;
    //if(bitSet)
    //{
    //    if(!even)
    //    {
    //        flags ^= FLAGS_P;
    //    }
    //}
    //else
    //{
    //    if(even)
    //    {
    //        flags ^= FLAGS_P;
    //    }
    //}

}

HandleInstructionResult HandleInstruction(Registers *registers, InstructionCode code, Operand leftOperand, Operand rightOperand)
{
    HandleInstructionResult result = {};
    result.prevFlags = flags;

    s16 regBefore = *GetRegister(registers, leftOperand.regCode);

    s16 *leftReg = GetRegister(registers, leftOperand.regCode);
    s16 *rightReg = GetRegister(registers, rightOperand.regCode);

    switch(code)
    {

    case Add:
    {
        if(rightOperand.opCode == Register)
        {
            *leftReg = *leftReg + *rightReg;
        }
        else if(rightOperand.opCode == Memory)
        {
            *leftReg = *leftReg + *rightReg;
        }
        else if(rightOperand.opCode == Immediate)
        {
            *leftReg = *leftReg + rightOperand.displacement;
        }
    } break;

    case Or:
    case Adc:
    case Sbb:
    case And:
    case Sub:
    {
        if(rightOperand.opCode == Register)
        {
            *leftReg = *leftReg - *rightReg;
        }
        else if(rightOperand.opCode == Memory)
        {
        }
        else if(rightOperand.opCode == Immediate)
        {
            *leftReg = *leftReg - rightOperand.displacement;
        }

        UpdateFlags(*leftReg);
    } break;

    case Xor:
    case Cmp:
    {
        s16 checkVal = 0;
        if(rightOperand.opCode == Register)
        {
            checkVal = *leftReg - *rightReg;
        }
        else if(rightOperand.opCode == Memory)
        {
        }
        else if(rightOperand.opCode == Immediate)
        {
            checkVal = *leftReg - rightOperand.displacement;
        }

        UpdateFlags(checkVal);
    } break;

    case Mov: 
    {
        if(rightOperand.opCode == Register)
        {
            *leftReg = *rightReg;
        }
        else if(rightOperand.opCode == Memory)
        {
        }
        else if(rightOperand.opCode == Immediate)
        {
            *leftReg = rightOperand.displacement;
        }
    } break;

    case Jo:
    case Jno:
    case Jb: 
    case Jnb: 
    case Je: 
    case Jne: 
    {
        bool bitSet = flags & FLAGS_Z;
        if(!bitSet)
        {
            //Note: -2 since at this point to read jump opcode we already read 2 bytes
            ip += leftOperand.displacement - 2;
        }
    } break;
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
    }
    break;

    default: break;
    }

    s16 regAfter = *GetRegister(registers, leftOperand.regCode);

    result.regBefore = regBefore;
    result.regAfter = regAfter;

    return result;
}

void PrintInstruction(InstructionCode code, Operand leftOperand, Operand rightOperand)
{
    char* instructionCode = GetInstructionCodeStr(code);
    char* leftOperandStr = GetRegCodeStr(leftOperand.regCode);
    char* rightOperandStr = GetRegCodeStr(rightOperand.regCode);

    fprintf(stdout, "%s ", instructionCode);

    switch(code)
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
        if(leftOperand.opCode == Register)
        {
            fprintf(stdout, "%s, ", leftOperandStr);
        }
        else if(leftOperand.opCode == Memory)
        {
            if(rightOperand.opCode == Immediate)
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
        else if(leftOperand.opCode == Immediate)
        {
            if(leftOperand.literals)
            {
                fprintf(stdout, "%s ", leftOperand.literals);
            }

            fprintf(stdout, "[%d], ", leftOperand.displacement);
        }

        if(rightOperand.opCode == Register)
        {
            fprintf(stdout, "%s", rightOperandStr);
        }
        else if(rightOperand.opCode == Memory)
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
        else if(rightOperand.opCode == Immediate)
        {
            fprintf(stdout, "%d", rightOperand.displacement);
        }
    } break;

    case Mov: 
    {
        if(leftOperand.opCode == Register)
        {
            fprintf(stdout, "%s, ", leftOperandStr);
        }
        else if(leftOperand.opCode == Memory)
        {
            fprintf(stdout, "[%s", leftOperandStr);

            if(leftOperand.displacement)
            {
                fprintf(stdout, " + %d", leftOperand.displacement);
            }

            fprintf(stdout, "], ");
        }
        else if(leftOperand.opCode == Immediate)
        {
            if(rightOperand.regCode == AL || rightOperand.regCode == AX)
            {
                fprintf(stdout, "[%d], ", leftOperand.displacement);
            }
            else
            {
                fprintf(stdout, "%d, ", leftOperand.displacement);
            }
        }

        if(rightOperand.opCode == Register)
        {
            fprintf(stdout, "%s", rightOperandStr);
        }
        else if(rightOperand.opCode == Memory)
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
        else if(rightOperand.opCode == Immediate)
        {
            if(leftOperand.regCode == AL || leftOperand.regCode == AX)
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
        fprintf(stdout, "$+%d", leftOperand.displacement);
    }
    break;


    default: break;
    }
}

void PrintRegister(Registers *regs, RegisterCode regCode)
{
    s16 reg = *GetRegister(regs, regCode);
    if(reg)
    {
        char *str = GetRegCodeStr(regCode);
        fprintf(stdout, "%10s: 0x%04hx (%d)\n", str, reg, (u16)reg);
    }
}

Instruction RegRom(InstructionCode instCode, u8 byte1, u8 byte2, u8* buffer, u8 wide, u8 reg, u8 imm)
{
    Instruction result = {};
    result.instCode = instCode;

    u8 dir = (byte1 >> 1) & 0b1;
    u8 mod = (byte2 >> 6) & 0b11;
    u8 rom = (byte2 >> 0) & 0b111;

    char* op = GetInstructionCodeStr(instCode);

    if(imm == 0b1)
    {
        s16 data = byte2;

        if(wide)
        {
            u8 byte3 = buffer[ip++];
            data = (byte3 << 8) | byte2;
        }

        result.operands[0].opCode = Register;
        result.operands[0].regCode = regTable[wide][reg];
        result.operands[1].opCode = Immediate;
        result.operands[1].displacement = data;
    }
    else
    {
        if(mod == 0b00)
        {
            if(rom == 0b110)
            {
                u8 byte3 = buffer[ip++];
                u8 byte4 = buffer[ip++];
                s16 data = (byte4 << 8) | byte3;

                result.operands[0].opCode = Register;
                result.operands[0].regCode = regTable[wide][reg];
                result.operands[1].opCode = Memory;
                result.operands[1].displacement = data;
            }
            else
            {   
                if(dir)
                {
                    result.operands[0].opCode = Register;
                    result.operands[0].regCode = regTable[wide][reg];
                    result.operands[1].opCode = Memory;
                    result.operands[1].regCode = romTable[rom];
                }
                else
                {
                    result.operands[0].opCode = Memory;
                    result.operands[0].regCode = romTable[rom];
                    result.operands[1].opCode = Register;
                    result.operands[1].regCode = regTable[wide][reg];
                }
            }
        }
        else if(mod == 0b01)
        {
            s8 data = buffer[ip++];

            if(dir)
            {
                result.operands[0].opCode = Register;
                result.operands[0].regCode = regTable[wide][reg];
                result.operands[1].opCode = Memory;
                result.operands[1].regCode = romTable[rom];
                result.operands[1].displacement = data;
            }
            else
            {
                result.operands[0].opCode = Memory;
                result.operands[0].regCode = romTable[rom];
                result.operands[0].displacement = data;
                result.operands[1].opCode = Register;
                result.operands[1].regCode = regTable[wide][reg];
            }
        }
        else if(mod == 0b10)
        {
            u8 byte3 = buffer[ip++];
            u8 byte4 = buffer[ip++];
            s16 data = (byte4 << 8) | byte3;

            if(dir)
            {
                result.operands[0].opCode = Register;
                result.operands[0].regCode = regTable[wide][reg];
                result.operands[1].opCode = Memory;
                result.operands[1].regCode = romTable[rom];
                result.operands[1].displacement = data;
            }
            else
            {
                result.operands[0].opCode = Memory;
                result.operands[0].regCode = romTable[rom];
                result.operands[0].displacement = data;
                result.operands[1].opCode = Register;
                result.operands[1].regCode = regTable[wide][reg];
            }
        }
        else if(mod == 0b11)
        {
            u8 src = dir ? reg : rom;
            u8 dest = dir ? rom : reg;

            result.operands[0].opCode = Register;
            result.operands[0].regCode = regTable[wide][src];
            result.operands[1].opCode = Register;
            result.operands[1].regCode = regTable[wide][dest];
        }
    }

    return result;
}

Instruction AddOrAdcSbbAndSubXorCmpMov(InstructionCode instCode, u8 byte1, u8 byte2, u8* buffer)
{
    Instruction result = {};
    result.instCode = instCode;

    u8 dir = (byte1 >> 1) & 0b1;
    u8 wide = (byte1 >> 0) & 0b1;
    u8 mod = (byte2 >> 6) & 0b11;
    u8 rom = (byte2 >> 0) & 0b111;

    bool mov = instCode == Mov;
    char* literals = wide ? "word" : "byte";
    char* op = GetInstructionCodeStr(instCode);

    if(mod == 0b00)
    {
        if(rom == 0b110)
        {
            u8 byte3 = buffer[ip++];
            
            result.operands[0].literals = literals;

            if(!dir && wide)
            {
                u8 byte4 = buffer[ip++];
                s16 data = (byte4 << 8) | byte3;

                result.operands[0].opCode = Immediate;
                result.operands[1].displacement = data;
                result.operands[1].opCode = Memory;
                result.operands[1].regCode = romTable[rom];
            }
            else if(dir && !wide)
            {
                s16 data = byte3;

                result.operands[0].opCode = Memory;
                result.operands[0].regCode = romTable[rom];
                result.operands[1].opCode = Immediate;
                result.operands[1].displacement = data;
            }
            else
            {
                u8 byte4 = buffer[ip++];
                u8 byte5 = buffer[ip++];
                s16 data = (byte4 << 8) | byte3;

                result.operands[0].opCode = Immediate;
                result.operands[0].displacement = data;
                result.operands[1].opCode = Immediate;
                result.operands[1].displacement = byte5;
            }
        }
        else
        {   
            u8 byte3 = buffer[ip++];
            s16 data = 0;

            int movTarget = mov ? 1 : 0;
            result.operands[0].opCode = Memory;
            result.operands[0].regCode = romTable[rom];
            result.operands[1].opCode = Immediate;
            result.operands[movTarget].literals = literals;

            if(!dir && wide)
            {
                u8 byte4 = buffer[ip++];
                data = (byte4 << 8) | byte3;
            }
            else
            {
                data = byte3;
            }

            result.operands[1].displacement = data;
        }
    }
    else if(mod == 0b01)
    {
        s16 data = 0;

        result.operands[0].opCode = Memory;
        result.operands[0].regCode = romTable[rom];
        result.operands[1].opCode = Immediate;

        if(mov)
        {
            s8 displacement = buffer[ip++];
            data = buffer[ip++];

            result.operands[0].displacement = displacement;
            result.operands[1].literals = literals;
        }
        else
        {
            u8 byte3 = buffer[ip++];
            u8 byte4 = buffer[ip++];
            u8 byte5 = buffer[ip++];
            s16 displacement = (byte4 << 8) | byte3;

            data = byte5;
            if(wide)
            {
                u8 byte6 = buffer[ip++];
                data = (byte6 << 8) | byte5;
            }

            result.operands[0].displacement = displacement;
            result.operands[0].literals = literals;
        }

        result.operands[1].displacement = data;

    }
    else if(mod == 0b10)
    {
        u8 byte3 = buffer[ip++];
        u8 byte4 = buffer[ip++];
        u8 byte5 = buffer[ip++];

        s16 displacement = (byte4 << 8) | byte3;
        s16 data = byte5;

        result.operands[0].opCode = Memory;
        result.operands[0].regCode = romTable[rom];
        result.operands[0].displacement = displacement;
        result.operands[1].opCode = Immediate;

        if(mov)
        {
            if(wide)
            {
                u8 byte6 = buffer[ip++];
                data = (byte6 << 8) | byte5;
            }

            result.operands[1].literals = literals;
        }
        else
        {
            result.operands[0].literals = literals;
        }

        result.operands[1].displacement = data;
    }
    else if(mod == 0b11)
    {
        u8 byte3 = buffer[ip++];
        s16 data = byte3;

        if(!dir && wide)
        {
            u8 byte4 = buffer[ip++];
            data = (byte4 << 8) | byte3;
        }

        result.operands[0].opCode = Memory;
        result.operands[0].regCode = regTable[wide][rom];
        result.operands[1].opCode = Immediate;
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
        fprintf(stdout, "--- test\\%s execution ---\n", targetFile);
    }
    else
    {
        fprintf(stdout, "bits 16\n\n");
    }
        
    while(ip < fileSize)
    {
        s16 prevIp = ip;

        Instruction instruction = {};

        u8 byte1 = buffer[ip++];
        u8 byte2 = buffer[ip++];

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
            instruction.instCode = Jo;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x71:
        {
            instruction.instCode = Jno;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x72:
        {
            instruction.instCode = Jb;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x73:
        {
            instruction.instCode = Jnb;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x74:
        {
            instruction.instCode = Je;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x75:
        {
            instruction.instCode = Jne;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x76:
        {
            instruction.instCode = Jbe;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x77:
        {
            instruction.instCode = Jnbe;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x78:
        {
            instruction.instCode = Js;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x79:
        {
            instruction.instCode = Jns;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x7a:
        {
            instruction.instCode = Jp;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x7b:
        {
            instruction.instCode = Jnp;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x7c:
        {
            instruction.instCode = Jl;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x7d:
        {
            instruction.instCode = Jnl;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x7e:
        {
            instruction.instCode = Jle;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0x7f:
        {
            instruction.instCode = Jnle;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
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
                u8 byte3 = buffer[ip++];
                data = (byte3 << 8) | byte2;
            }
            
            instruction.instCode = Mov;
                                          
            if(dir)
            {
                instruction.operands[0].opCode = Immediate;
                instruction.operands[0].displacement = data;
                instruction.operands[1].opCode = Register;
                instruction.operands[1].regCode = regTable[wide][reg];
            }
            else
            {
                instruction.operands[0].opCode = Register;
                instruction.operands[0].regCode = regTable[wide][reg];
                instruction.operands[1].opCode = Immediate;
                instruction.operands[1].displacement = data;
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
            instruction.instCode = Loopne;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0xe1:
        {
            instruction.instCode = Loope;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0xe2:
        {
            instruction.instCode = Loop;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;

        case 0xe3:
        {
            instruction.instCode = Jcxz;
            instruction.operands[0].displacement = (s8)byte2 + 2;
            instruction.operands[0].regCode = IP;
        } break;


        default: 
        {
            printf("0x%x unimplemented\n", byte1);
        } break;

        }

        Operand leftOperand = instruction.operands[0];
        Operand rightOperand = instruction.operands[1];

        char* leftOperandStr = GetRegCodeStr(leftOperand.regCode);
        char* rightOperandStr = GetRegCodeStr(rightOperand.regCode);

        if(executionMode)
        {
            HandleInstructionResult instructionResult = HandleInstruction(&regs, instruction.instCode, leftOperand, rightOperand);
            s16 prevFlags = instructionResult.prevFlags;

            PrintInstruction(instruction.instCode, leftOperand, rightOperand);

            char *regCode = GetRegCodeStr(leftOperand.regCode);
            fprintf(stdout, "; %s:0x%04hx->0x%04hx ip:0x%04hx->0x%04hx", 
                    regCode, 
                    instructionResult.regBefore, 
                    instructionResult.regAfter,
                    prevIp,
                    ip);

            if(prevFlags != flags)
            {
                fprintf(stdout, " flags:");

                for(int index = 0; index < FLAGS_COUNT; ++index)
                {
                    int bitVal = 1 << index;
                    bool bitPreviouslySet = prevFlags & bitVal;
                    bool bitCleared = (flags & bitVal) == 0;
                    if(bitPreviouslySet && bitCleared)
                    {
                        char* flagStr = GetFlagsStr(bitVal);
                        fprintf(stdout, "%s", flagStr);
                    }
                }

                fprintf(stdout, "->");

                for(int index = 0; index < FLAGS_COUNT; ++index)
                {
                    int bitVal = 1 << index;
                    bool bitPreviouslyUnset = (prevFlags & bitVal) == 0;
                    bool bitNowSet = flags & bitVal;
                    if(bitPreviouslyUnset && bitNowSet)
                    {
                        char* flagStr = GetFlagsStr(bitVal);
                        fprintf(stdout, "%s", flagStr);
                    }
                }
            }

            fprintf(stdout, "\n");
        }
        else
        {
            PrintInstruction(instruction.instCode, leftOperand, rightOperand);
            fprintf(stdout, "\n");
        }
    }

    if(executionMode)
    {
        fprintf(stdout, "\nFinal registers:\n");
        PrintRegister(&regs, AX);
        PrintRegister(&regs, BX);
        PrintRegister(&regs, CX);
        PrintRegister(&regs, DX);
        PrintRegister(&regs, SP);
        PrintRegister(&regs, BP);
        PrintRegister(&regs, SI);
        PrintRegister(&regs, DI);
        fprintf(stdout, "%10s: 0x%04hx (%d)\n", "ip", ip, (u16)ip);
        fprintf(stdout, "%10s: ", "flags");
        for(int index = 0; index < FLAGS_COUNT; ++index)
        {
            int bitVal = 1 << index;
            bool bitSet = flags & bitVal;
            if(bitSet)
            {
                char* flagStr = GetFlagsStr(bitVal);
                fprintf(stdout, "%s", flagStr);
            }
        }
        fprintf(stdout, "\n");
    }

    return 0;
}
