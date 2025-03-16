#ifndef __MIPS_OP_H__ 
#define __MIPS_OP_H__ 


#define  MIPS32_OP_SPECIAL   0b000000 
#define  MIPS32_OP_REGIMM    0b000001 
#define  MIPS32_OP_J         0b000010 
#define  MIPS32_OP_JAL       0b000011 
#define  MIPS32_OP_BEQ       0b000100 
#define  MIPS32_OP_BNE       0b000101 
#define  MIPS32_OP_BLEZ      0b000110 
#define  MIPS32_OP_BGTZ      0b000111 
#define  MIPS32_OP_ADDI      0b001000 
#define  MIPS32_OP_ADDIU     0b001001 
#define  MIPS32_OP_SLTI      0b001010 
#define  MIPS32_OP_SLTIU     0b001011 
#define  MIPS32_OP_ANDI      0b001100 
#define  MIPS32_OP_ORI       0b001101 
#define  MIPS32_OP_XORI      0b001110 
#define  MIPS32_OP_LUI       0b001111 
#define  MIPS32_OP_COP0      0b010000 
#define  MIPS32_OP_COP1      0b010001 
#define  MIPS32_OP_COP2      0b010010 
#define  MIPS32_OP_COP1X     0b010011 
#define  MIPS32_OP_BEQL      0b010100 
#define  MIPS32_OP_BNEL      0b010101 
#define  MIPS32_OP_BLEZL     0b010110 
#define  MIPS32_OP_BGTZL     0b010111 
//#define  MIPS32_OP_POP30     0b011_000 
#define  MIPS32_OP_SPECIAL2  0b011100 
#define  MIPS32_OP_JALX      0b011101 
#define  MIPS32_OP_MSA       0b011110 
#define  MIPS32_OP_SPECIAL3  0b011111 
#define  MIPS32_OP_LB        0b100000 
#define  MIPS32_OP_LH        0b100001 
#define  MIPS32_OP_LWL       0b100010 
#define  MIPS32_OP_LW        0b100011 
#define  MIPS32_OP_LBU       0b100100 
#define  MIPS32_OP_LHU       0b100101 
#define  MIPS32_OP_LWR       0b100110 
#define  MIPS32_OP_SB        0b101000 
#define  MIPS32_OP_SH        0b101001 
#define  MIPS32_OP_SWL       0b101010 
#define  MIPS32_OP_SW        0b101011 
#define  MIPS32_OP_SWR       0b101110 
#define  MIPS32_OP_CACHE     0b101111 
#define  MIPS32_OP_LL        0b110000 
#define  MIPS32_OP_LWC1      0b110001 
#define  MIPS32_OP_LWC2      0b110010 
#define  MIPS32_OP_PREF      0b110011 
#define  MIPS32_OP_LDC1      0b110101 
#define  MIPS32_OP_LDC2      0b110110 
#define  MIPS32_OP_SC        0b111000 
#define  MIPS32_OP_SWC1      0b111001 
#define  MIPS32_OP_SWC2      0b111010 
#define  MIPS32_OP_PCREL     0b111011 /* not used in MIPS32R2 */
#define  MIPS32_OP_SDC1      0b111101 
#define  MIPS32_OP_SDC2      0b111110 




#define  MIPS16E_OP_ADDIUSP   0x00 
#define  MIPS16E_OP_ADDIUPC   0x01 
#define  MIPS16E_OP_B         0x02 
#define  MIPS16E_OP_JAL       0x03 
#define  MIPS16E_OP_BEQZ      0x04 
#define  MIPS16E_OP_BNEZ      0x05 
#define  MIPS16E_OP_SHIFT     0x06 
#define  MIPS16E_OP_RRIA      0x08 
#define  MIPS16E_OP_ADDIU8    0x09 
#define  MIPS16E_OP_SLTI      0x0A 
#define  MIPS16E_OP_SLTIU     0x0B 
#define  MIPS16E_OP_I8        0x0C 
#define  MIPS16E_OP_LI        0x0D 
#define  MIPS16E_OP_CMPI      0x0E 
#define  MIPS16E_OP_LB        0x10 
#define  MIPS16E_OP_LH        0x11 
#define  MIPS16E_OP_LWSP      0x12 
#define  MIPS16E_OP_LW        0x13 
#define  MIPS16E_OP_LBU       0x14 
#define  MIPS16E_OP_LHU       0x15 
#define  MIPS16E_OP_LWPC      0x16 
#define  MIPS16E_OP_SB        0x18 
#define  MIPS16E_OP_SH        0x19 
#define  MIPS16E_OP_SWSP      0x1A 
#define  MIPS16E_OP_SW        0x1B 
#define  MIPS16E_OP_RRR       0x1C 
#define  MIPS16E_OP_RR        0x1D 
#define  MIPS16E_OP_EXTEND    0x1E 

#define  MIPS16E_RRFUNCT_JR       0x00 
#define  MIPS16E_RRFUNCT_SDBBP    0x01 
#define  MIPS16E_RRFUNCT_SLT      0x02 
#define  MIPS16E_RRFUNCT_SLTU     0x03 
#define  MIPS16E_RRFUNCT_SLLV     0x04 
#define  MIPS16E_RRFUNCT_BREAK    0x05 
#define  MIPS16E_RRFUNCT_SRLV     0x06 
#define  MIPS16E_RRFUNCT_SRAV     0x07 
#define  MIPS16E_RRFUNCT_CMP      0x0A 
#define  MIPS16E_RRFUNCT_NEG      0x0B 
#define  MIPS16E_RRFUNCT_AND      0x0C 
#define  MIPS16E_RRFUNCT_OR       0x0D 
#define  MIPS16E_RRFUNCT_XOR      0x0E 
#define  MIPS16E_RRFUNCT_NOT      0x0F 
#define  MIPS16E_RRFUNCT_MFHI     0x10 
#define  MIPS16E_RRFUNCT_CNVT     0x11 
#define  MIPS16E_RRFUNCT_MFLO     0x12 
#define  MIPS16E_RRFUNCT_MULT     0x18 
#define  MIPS16E_RRFUNCT_MULTU    0x19 
#define  MIPS16E_RRFUNCT_DIV      0x1A 
#define  MIPS16E_RRFUNCT_DIVU     0x1B 


#endif
