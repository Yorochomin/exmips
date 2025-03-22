#ifndef __RC_X64_H__ 
#define __RC_X64_H__ 

#include "mips.h"

#define REG_NUM_AX 0
#define REG_NUM_CX 1
#define REG_NUM_DX 2
#define REG_NUM_BX 3
#define REG_NUM_SP 4
#define REG_NUM_BP 5
#define REG_NUM_SI 6
#define REG_NUM_DI 7


#define gen_ret(mem, pused)   do{ (mem)[*(pused)] = 0xc3; *(pused)+=1; }while(0)

#define gen_push_reg(mem, pused, reg)  do{ (mem)[*(pused)] = 0x50+(reg); *(pused)+=1; }while(0)
#define gen_pop_reg(mem, pused, reg)   do{ (mem)[*(pused)] = 0x58+(reg); *(pused)+=1; }while(0)
#define gen_prefix_rex64(mem, pused)   do{ (mem)[*(pused)] = 0x48;       *(pused)+=1; }while(0)

#define __gen_op_reg_reg(mem, pused, op, rd, rs)  \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0xc0+((rd)<<3)+(rs); *(pused)+=2; }while(0)
#define __gen_op_reg_sidx8_base(mem, pused, op, rd, idx, rb) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0x40+((rd)<<3)+(rb);  \
                                (mem)[*(pused)+2] = (idx); *(pused)+=3; }while(0)
#define __gen_op_reg_idx32_base(mem, pused, op, rd, idx, rb) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0x80+((rd)<<3)+(rb); \
                                (mem)[*(pused)+2] = (((idx)>> 0)&0xff); (mem)[*(pused)+3] = (((idx)>> 8)&0xff); \
                                (mem)[*(pused)+4] = (((idx)>>16)&0xff); (mem)[*(pused)+5] = (((idx)>>24)&0xff); *(pused)+=6; }while(0)
  
#define gen_mov_reg_to_reg(mem, pused, rd, rs)             __gen_op_reg_reg       (mem, pused, 0x8b, rd, rs)

#define gen_mov_sidx8_base_to_reg(mem, pused, rd, idx, rb) __gen_op_reg_sidx8_base(mem, pused, 0x8b, rd, idx, rb)
#define gen_mov_reg_to_sidx8_base(mem, pused, rs, idx, rb) __gen_op_reg_sidx8_base(mem, pused, 0x89, rs, idx, rb)

#define gen_mov_idx32_base_to_reg(mem, pused, rd, idx, rb) __gen_op_reg_idx32_base(mem, pused, 0x8b, rd, idx, rb)
#define gen_mov_reg_to_idx32_base(mem, pused, rs, idx, rb) __gen_op_reg_idx32_base(mem, pused, 0x89, rs, idx, rb)


#define gen_add_reg_reg(       mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x03, rd, rs)
#define gen_add_reg_sidx8_base(mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x03, rd, idx, rb)

#define gen_adc_reg_reg(       mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x13, rd, rs)
#define gen_adc_reg_sidx8_base(mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x13, rd, idx, rb)

#define gen_sub_reg_reg(       mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x2b, rd, rs)
#define gen_sub_reg_sidx8_base(mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x2b, rd, idx, rb)

#define gen_cmp_reg_reg(       mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x3b, rd, rs)
#define gen_cmp_reg_sidx8_base(mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x3b, rd, idx, rb)

#define gen_and_reg_reg(       mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x23, rd, rs)
#define gen_and_reg_sidx8_base(mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x23, rd, idx, rb)
#define gen_and_sidx8_base_reg(mem, pused, idx, rb, rs)    __gen_op_reg_sidx8_base(mem, pused, 0x21, rs, idx, rb)

#define gen_or_reg_reg(        mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x0b, rd, rs)
#define gen_or_reg_sidx8_base( mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x0b, rd, idx, rb)
#define gen_or_sidx8_base_reg( mem, pused, idx, rb, rs)    __gen_op_reg_sidx8_base(mem, pused, 0x09, rs, idx, rb)

#define gen_xor_reg_reg(       mem, pused, rd, rs)         __gen_op_reg_reg(       mem, pused, 0x33, rd, rs)
#define gen_xor_reg_sidx8_base(mem, pused, rd, idx, rb)    __gen_op_reg_sidx8_base(mem, pused, 0x33, rd, idx, rb)
#define gen_xor_sidx8_base_reg(mem, pused, idx, rb, rs)    __gen_op_reg_sidx8_base(mem, pused, 0x31, rs, idx, rb)

#define gen_test_reg_reg(       mem, pused, rd, rs)        __gen_op_reg_reg(       mem, pused, 0x85, rd, rs)
#define gen_test_reg_sidx8_base(mem, pused, rd, idx, rb)   __gen_op_reg_sidx8_base(mem, pused, 0x85, rd, idx, rb)


#define __gen_f7_reg(mem, pused, op, reg)  \
                                do{ (mem)[*(pused)] = 0xf7; (mem)[*(pused)+1] = 0xc0+((op)<<3)+(reg); *(pused)+=2; }while(0)

#define __gen_f7_sidx8_base(mem, pused, op, idx, rb) \
                                do{ (mem)[*(pused)] = 0xf7; (mem)[*(pused)+1] = 0x40+((op)<<3)+(rb);  \
                                (mem)[*(pused)+2] = (idx); *(pused)+=3; }while(0)
  
#define gen_imul_reg_reg(mem, pused, rd, rs) \
                              do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = 0xaf; (mem)[*(pused)+2] = 0xc0+((rd)<<3)+(rs); *(pused)+=3; }while(0)

#define gen_bsr_reg_reg(mem, pused, rd, rs) \
                              do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = 0xbd; (mem)[*(pused)+2] = 0xc0+((rd)<<3)+(rs); *(pused)+=3; }while(0)


#define gen_not_reg(mem, pused, rd) __gen_f7_reg(mem, pused, 2, rd)
#define gen_neg_reg(mem, pused, rd) __gen_f7_reg(mem, pused, 3, rd)
                              
/* unsigned multiply. EDX:EAX <- EAX * reg */
#define gen_mul(mem, pused, reg)    __gen_f7_reg(mem, pused, 4, reg)

/* signed multiply. EDX:EAX <- EAX * reg */
#define gen_imul(mem, pused, reg)   __gen_f7_reg(mem, pused, 5, reg)

/* unsigned division. EDX:EAX / reg -> EAX: quotient, EDX: remainder */
#define gen_div(mem, pused, reg)    __gen_f7_reg(mem, pused, 6, reg)

/* signed division. EDX:EAX / reg -> EAX: quotient, EDX: remainder */
#define gen_idiv(mem, pused, reg)   __gen_f7_reg(mem, pused, 7, reg)


#define __gen_op_ax_imm8(mem, pused, op, imm8)  do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = (imm8); *(pused)+=2; }while(0)
#define __gen_op_ax_imm32(mem, pused, op, imm) \
                                do{ (mem)[*(pused)] = (op); \
                                (mem)[*(pused)+1] = (((imm)>> 0)&0xff); (mem)[*(pused)+2] = (((imm)>> 8)&0xff); \
                                (mem)[*(pused)+3] = (((imm)>>16)&0xff); (mem)[*(pused)+4] = (((imm)>>24)&0xff); *(pused)+=5; }while(0)

#define gen_add_ax_imm8( mem, pused, imm8)  __gen_op_ax_imm8( mem, pused, 0x04, imm8)
#define gen_add_ax_imm32(mem, pused, imm )  __gen_op_ax_imm32(mem, pused, 0x05, imm )

#define gen_and_ax_imm8( mem, pused, imm8)  __gen_op_ax_imm8( mem, pused, 0x24, imm8)
#define gen_and_ax_imm32(mem, pused, imm )  __gen_op_ax_imm32(mem, pused, 0x25, imm )


#define __gen_op_reg8_imm8(mem, pused, op, op3bit, rd, imm8) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0xc0+((op3bit)<<3)+(rd); \
                                (mem)[*(pused)+2] = (imm8); *(pused)+=3; }while(0)

#define __gen_op_reg_imm32(mem, pused, op, op3bit, rd, imm) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0xc0+((op3bit)<<3)+(rd); \
                                (mem)[*(pused)+2] = (((imm)>> 0)&0xff); (mem)[*(pused)+3] = (((imm)>> 8)&0xff); \
                                (mem)[*(pused)+4] = (((imm)>>16)&0xff); (mem)[*(pused)+5] = (((imm)>>24)&0xff); *(pused)+=6; }while(0)

#define __gen_op_sidx8_base_imm32(mem, pused, op, op3bit, idx, rb, imm) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0x40+((op3bit)<<3)+(rb); (mem)[*(pused)+2] = (idx); \
                                (mem)[*(pused)+3] = (((imm)>> 0)&0xff); (mem)[*(pused)+4] = (((imm)>> 8)&0xff); \
                                (mem)[*(pused)+5] = (((imm)>>16)&0xff); (mem)[*(pused)+6] = (((imm)>>24)&0xff); *(pused)+=7; }while(0)

#define gen_add_reg8_imm8(mem, pused, rd, imm)              __gen_op_reg8_imm8(mem, pused, 0x80, 0x00, rd, imm)
#define gen_add_reg_imm32(mem, pused, rd, imm)              __gen_op_reg_imm32(mem, pused, 0x81, 0x00, rd, imm)
#define gen_add_reg_sign_ext_imm8(mem, pused, rd, imm)      __gen_op_reg8_imm8(mem, pused, 0x83, 0x00, rd, imm)
#define gen_add_sidx8_base_imm32(mem, pused, idx, rb, imm)  __gen_op_sidx8_base_imm32(mem, pused, 0x81, 0x00, idx, rb, imm)

#define gen_and_reg8_imm8(mem, pused, rd, imm)              __gen_op_reg8_imm8(mem, pused, 0x80, 0x04, rd, imm)
#define gen_and_reg_imm32(mem, pused, rd, imm)              __gen_op_reg_imm32(mem, pused, 0x81, 0x04, rd, imm)
#define gen_and_reg_sign_ext_imm8(mem, pused, rd, imm)      __gen_op_reg8_imm8(mem, pused, 0x83, 0x04, rd, imm)

#define gen_sub_reg8_imm8(mem, pused, rd, imm)              __gen_op_reg8_imm8(mem, pused, 0x80, 0x05, rd, imm)
#define gen_sub_reg_imm32(mem, pused, rd, imm)              __gen_op_reg_imm32(mem, pused, 0x81, 0x05, rd, imm)
#define gen_sub_reg_sign_ext_imm8(mem, pused, rd, imm)      __gen_op_reg8_imm8(mem, pused, 0x83, 0x05, rd, imm)

#define gen_xor_reg8_imm8(mem, pused, rd, imm)              __gen_op_reg8_imm8(mem, pused, 0x80, 0x06, rd, imm)
#define gen_xor_reg_imm32(mem, pused, rd, imm)              __gen_op_reg_imm32(mem, pused, 0x81, 0x06, rd, imm)
#define gen_xor_reg_sign_ext_imm8(mem, pused, rd, imm)      __gen_op_reg8_imm8(mem, pused, 0x83, 0x06, rd, imm)

#define gen_cmp_reg8_imm8(mem, pused, rd, imm)              __gen_op_reg8_imm8(mem, pused, 0x80, 0x07, rd, imm)
#define gen_cmp_reg_imm32(mem, pused, rd, imm)              __gen_op_reg_imm32(mem, pused, 0x81, 0x07, rd, imm)
#define gen_cmp_reg_sign_ext_imm8(mem, pused, rd, imm)      __gen_op_reg8_imm8(mem, pused, 0x83, 0x07, rd, imm)
#define gen_cmp_sidx8_base_imm32(mem, pused, idx, rb, imm)  __gen_op_sidx8_base_imm32(mem, pused, 0x81, 0x07, idx, rb, imm)


#define __gen_op_reg(mem, pused, op, op3bit, rd) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0xc0+((op3bit)<<3)+(rd); *(pused)+=2; }while(0)
#define __gen_op_sidx8_base(mem, pused, op, op3bit, idx, rb) \
                                do{ (mem)[*(pused)] = (op); (mem)[*(pused)+1] = 0x40+((op3bit)<<3)+(rb); \
                                (mem)[*(pused)+2] = (idx); *(pused)+=3; }while(0)


/* INC instruction */
#define gen_inc_reg(          mem, pused, rd)      __gen_op_reg(       mem, pused, 0xff, 0x00, rd)

/* x86 SHL instruction*/
#define gen_sll_reg_cl(       mem, pused, rd)      __gen_op_reg(       mem, pused, 0xd3, 0x04, rd)
#define gen_sll_reg_imm8(mem, pused, rd, imm8)     __gen_op_reg8_imm8( mem, pused, 0xc1, 0x04, rd, imm8) 
#define gen_sll_sidx8_base_cl(mem, pused, idx, rb) __gen_op_sidx8_base(mem, pused, 0xd3, 0x04, idx, rb)

/* x86 SHR instruction*/
#define gen_srl_reg_cl(mem, pused, rd)             __gen_op_reg(       mem, pused, 0xd3, 0x05, rd)
#define gen_srl_reg_imm8(mem, pused, rd, imm8)     __gen_op_reg8_imm8( mem, pused, 0xc1, 0x05, rd, imm8) 
#define gen_srl_sidx8_base_cl(mem, pused, idx, rb) __gen_op_sidx8_base(mem, pused, 0xd3, 0x05, idx, rb)

/* x86 SAR instruction*/
#define gen_sar_reg_cl(mem, pused, rd)             __gen_op_reg(       mem, pused, 0xd3, 0x07, rd)
#define gen_sar_reg_imm8(mem, pused, rd, imm8)     __gen_op_reg8_imm8( mem, pused, 0xc1, 0x07, rd, imm8) 
#define gen_sar_sidx8_base_cl(mem, pused, idx, rb) __gen_op_sidx8_base(mem, pused, 0xd3, 0x07, idx, rb)

/* x86 ROR instruction*/
#define gen_ror_reg_cl(mem, pused, rd)             __gen_op_reg(       mem, pused, 0xd3, 0x01, rd)
#define gen_ror_reg_imm8(mem, pused, rd, imm8)     __gen_op_reg8_imm8( mem, pused, 0xc1, 0x01, rd, imm8) 
#define gen_ror_sidx8_base_cl(mem, pused, idx, rb) __gen_op_sidx8_base(mem, pused, 0xd3, 0x01, idx, rb)

#define gen_mov_imm32_to_reg(mem, pused, rd, imm) \
                              do{ (mem)[*(pused)] = 0xb8+(rd); \
                              (mem)[*(pused)+1] = (((imm)>> 0)&0xff); (mem)[*(pused)+2] = (((imm)>> 8)&0xff); \
                              (mem)[*(pused)+3] = (((imm)>>16)&0xff); (mem)[*(pused)+4] = (((imm)>>24)&0xff); *(pused)+=5; }while(0)

#define gen_mov_imm32_to_sidx8_base(mem, pused, idx, rb, imm) \
                              do{ (mem)[*(pused)] = 0xc7; (mem)[*(pused)+1] = 0x40+ (0<<3)+(rb); (mem)[*(pused)+2] = (idx); \
                              (mem)[*(pused)+3] = (((imm)>> 0)&0xff); (mem)[*(pused)+4] = (((imm)>> 8)&0xff); \
                              (mem)[*(pused)+5] = (((imm)>>16)&0xff); (mem)[*(pused)+6] = (((imm)>>24)&0xff); *(pused)+=7; }while(0)

#define gen_mov_imm32_to_idx32_base(mem, pused, idx, rb, imm) \
                              do{ (mem)[*(pused)] = 0xc7; (mem)[*(pused)+1] = 0x80+ (0<<3)+(rb); \
                              (mem)[*(pused)+2] = (((idx)>> 0)&0xff); (mem)[*(pused)+3] = (((idx)>> 8)&0xff); \
                              (mem)[*(pused)+4] = (((idx)>>16)&0xff); (mem)[*(pused)+5] = (((idx)>>24)&0xff); \
                              (mem)[*(pused)+6] = (((imm)>> 0)&0xff); (mem)[*(pused)+7] = (((imm)>> 8)&0xff); \
                              (mem)[*(pused)+8] = (((imm)>>16)&0xff); (mem)[*(pused)+9] = (((imm)>>24)&0xff); *(pused)+=10; }while(0)
  
#define gen_mov_imm8_to_reg8(mem, pused, rd8, imm8) \
                              do{ (mem)[*(pused)] = 0xb0+(rd8); (mem)[*(pused)+1] = (imm8); *(pused)+=2; }while(0)

#define gen_call_reg(mem, pused, reg) \
                              do{ (mem)[*(pused)] = 0xff; (mem)[*(pused)+1] = (0xc0 | (2<<3) | (reg)); *(pused)+=2; }while(0)

#define gen_call_rel_pc(mem, pused, rel_pc) /* rel_pc + next ip = target ip */\
                              do{ (mem)[*(pused)] = 0xe8; \
                              (mem)[*(pused)+1] = (((rel_pc)>> 0)&0xff); (mem)[*(pused)+2] = (((rel_pc)>> 8)&0xff); \
                              (mem)[*(pused)+3] = (((rel_pc)>>16)&0xff); (mem)[*(pused)+4] = (((rel_pc)>>24)&0xff); *(pused)+=5; }while(0)

#define gen_jmp_rel_pc(mem, pused, rel_pc8) /* rel_pc(sign extended) + next ip = target ip */\
                              do{ (mem)[*(pused)] = 0xeb; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_jz_rel_pc(mem, pused, rel_pc8) /* rel_pc(sign extended) + next ip = target ip */\
                              do{ (mem)[*(pused)] = 0x74; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_jnz_rel_pc(mem, pused, rel_pc8) /* rel_pc(sign extended) + next ip = target ip */\
                              do{ (mem)[*(pused)] = 0x75; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_jl_rel_pc(mem, pused, rel_pc8) /* (jnge), signed-comparison */\
                              do{ (mem)[*(pused)] = 0x7c; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_jge_rel_pc(mem, pused, rel_pc8) /* (jnl), signed-comparison */\
                              do{ (mem)[*(pused)] = 0x7d; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_jle_rel_pc(mem, pused, rel_pc8) /* (jng), signed-comparison */\
                              do{ (mem)[*(pused)] = 0x7e; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_jg_rel_pc(mem, pused, rel_pc8) /* (jnle), signed-comparison */\
                              do{ (mem)[*(pused)] = 0x7f; (mem)[*(pused)+1] = (rel_pc8); *(pused)+=2; }while(0)

#define gen_setb_reg(mem, pused, rd) /* CF = 1 */ \
                              do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = 0x92; (mem)[*(pused)+2] = 0xc0+ (rd); *(pused)+=3; }while(0)

#define gen_setl_reg(mem, pused, rd) /* SF != OF */ \
                              do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = 0x9c; (mem)[*(pused)+2] = 0xc0+ (rd); *(pused)+=3; }while(0)


#define __gen_0f_op_reg_reg(mem, pused, op, rd, rs)  \
                              do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = (op); (mem)[*(pused)+2] = 0xc0+((rd)<<3)+(rs); *(pused)+=3; }while(0)
#define __gen_0f_op_reg_sidx8_base(mem, pused, op, rd, idx, rb) \
                              do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = (op); (mem)[*(pused)+2] = 0x40+((rd)<<3)+(rb);  \
                              (mem)[*(pused)+3] = (idx); *(pused)+=4; }while(0)

/* x86 CMOVNE (move if zf=0) */
#define gen_cmovne_reg_reg(       mem, pused, rd, rs)      __gen_0f_op_reg_reg       (mem, pused, 0x45, rd, rs)
#define gen_cmovne_reg_sidx8_base(mem, pused, rd, idx, rb) __gen_0f_op_reg_sidx8_base(mem, pused, 0x45, rd, idx, rb)

/* x86 CMOVE (move if zf=1) */
#define gen_cmove_reg_reg(        mem, pused, rd, rs)      __gen_0f_op_reg_reg       (mem, pused, 0x44, rd, rs)
#define gen_cmove_reg_sidx8_base(mem, pused, rd, idx, rb)  __gen_0f_op_reg_sidx8_base(mem, pused, 0x44, rd, idx, rb)

#define gen_cwde(mem, pused) /* sign extension of AX -> EAX */ \
        do{ (mem)[*(pused)] = 0x98; *(pused)+=1; }while(0)

#define gen_movsx_sidx8_base_to_reg(mem, pused, rd, idx, rb) /* sign extension from byte -> r32 */ \
        do{ (mem)[*(pused)] = 0x0f; (mem)[*(pused)+1] = 0xbe; (mem)[*(pused)+2] = 0x40+((rd)<<3)+(rb); \
            (mem)[*(pused)+3] = (idx); *(pused)+=4; }while(0)


static inline void gen_load_insts(struct stMachineState *pM, uint8_t *mem, int *pused, uint8_t *imem, uintmx pointer, uint32_t pc_offset, uint32_t reg_dest, uint32_t reg_base, uint32_t offset, uint64_t func){
    uint64_t utmp;
    // Set PC
    gen_mov_imm32_to_idx32_base(mem, pused, pc_offset, REG_NUM_DI, pointer);

    gen_push_reg             (mem, pused, REG_NUM_SI);
    gen_push_reg             (mem, pused, REG_NUM_DI);

    gen_push_reg             (mem, pused, REG_NUM_DI); // dummy-push for 16-byte alignment rule of stack-pointer (push=8x2 and return address= 8 = 24, we need 8)
/*
    gen_prefix_rex64         (mem, pused);
    gen_sub_reg_sign_ext_imm8(mem, pused, REG_NUM_SP, 8); // for 16-byte alignment rule of stack-pointer (push=8x2 and return address= 8 = 24, we need 8)
*/

    // argument : edi, dsi, edx
    if( reg_base != 0 ){
        gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_SI, reg_base*4, REG_NUM_SI);
        gen_add_reg_imm32        (mem, pused, REG_NUM_SI, offset);
    }else{
        gen_mov_imm32_to_reg     (mem, pused, REG_NUM_SI, offset);
    }
    gen_xor_reg_reg          (mem, pused, REG_NUM_DX, REG_NUM_DX);
    gen_mov_imm8_to_reg8     (mem, pused, REG_NUM_DX, reg_dest);
    utmp = func - ((uint64_t)&imem[*pused+5]);
    gen_call_rel_pc          (mem, pused, utmp);

/*
    gen_prefix_rex64         (mem, pused);
    gen_add_reg_sign_ext_imm8(mem, pused, REG_NUM_SP, 8);
*/
    gen_pop_reg              (mem, pused, REG_NUM_DI);

    gen_pop_reg              (mem, pused, REG_NUM_DI);
    gen_pop_reg              (mem, pused, REG_NUM_SI);
    gen_test_reg_reg         (mem, pused, REG_NUM_AX, REG_NUM_AX); // if cx=0, zf will be 1.
    gen_jz_rel_pc            (mem, pused, 3);
    // processing for exception
    gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
    gen_ret                  (mem, pused); // 1byte
}

static inline void gen_store_insts(struct stMachineState *pM, uint8_t *mem, int *pused, uint8_t *imem, uintmx pointer, uint32_t pc_offset, uint32_t reg_src, uint32_t reg_base, uint32_t offset, uint64_t func){
    uint64_t utmp;
    // Set PC
    gen_mov_imm32_to_idx32_base(mem,pused, pc_offset, REG_NUM_DI, pointer);

    gen_push_reg             (mem, pused, REG_NUM_SI);
    gen_push_reg             (mem, pused, REG_NUM_DI);

    gen_push_reg             (mem, pused, REG_NUM_DI); // dummy-push for 16-byte alignment rule of stack-pointer (push=8x2 and return address= 8 = 24, we need 8)
/*
    gen_prefix_rex64         (mem, pused);
    gen_sub_reg_sign_ext_imm8(mem, pused, REG_NUM_SP, 8); // for 16-byte alignment rule of stack-pointer (push=8x2 and return address= 8 = 24, we need 8)
*/

    // argument : edi, dsi, edx
    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_DX, reg_src *4, REG_NUM_SI);
    gen_mov_sidx8_base_to_reg(mem, pused, REG_NUM_SI, reg_base*4, REG_NUM_SI);
    gen_add_reg_imm32        (mem, pused, REG_NUM_SI, offset);
    utmp = func - ((uint64_t)&imem[*pused+5]);
    gen_call_rel_pc          (mem, pused, utmp);

/*
    gen_prefix_rex64         (mem, pused);
    gen_add_reg_sign_ext_imm8(mem, pused, REG_NUM_SP, 8);
*/
    gen_pop_reg              (mem, pused, REG_NUM_DI);

    gen_pop_reg              (mem, pused, REG_NUM_DI);
    gen_pop_reg              (mem, pused, REG_NUM_SI);
    gen_test_reg_reg         (mem, pused, REG_NUM_AX, REG_NUM_AX); // if cx=0, zf will be 1.
    gen_jz_rel_pc            (mem, pused, 3);
    // processing for exception
    gen_xor_reg_reg          (mem, pused, REG_NUM_AX, REG_NUM_AX);
    gen_ret                  (mem, pused); // 1byte
}

int compile16(struct stMachineState *pM, uint8_t *imem, uintmx vaddr);
int compile32(struct stMachineState *pM, uint8_t *imem, uintmx vaddr);


#endif
