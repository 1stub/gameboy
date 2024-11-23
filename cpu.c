#include "cpu.h"
#include "mmu.h"

CPU cpu;

#define DO_NOTHING -1
#define RESET 0
#define SET 1

static byte cycles[0x100];
static const byte pc_inc[0x100];
static const byte extended_cycles[0x100];
static const byte extended_pc_inc[0x100];

//-1 == do nothing, 0 == reset, else == set
static inline void SET_FLAGS(int z, int n, int h, int c){
    if (z == RESET) F &= ~(FLAG_Z);
    else if (z == SET) F |= FLAG_Z;

    if (n == RESET) F &= ~(FLAG_N);
    else if (n == SET) F |= FLAG_N;

    if (h == RESET) F &= ~(FLAG_H); 
    else if (h == SET) F |= FLAG_H;

    if (c == RESET) F &= ~(FLAG_C);
    else if (c == SET) F |= FLAG_C;
}

void cpu_init(){
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
    PC = 0x100;
    SP = 0xFFFE; 
    IME = 0;
    cpu.cycles = 0;
    cpu.is_halted = 0;
}

//our cpu will cycle in increments of 4 T cycles (1 Machine Cycle) 
byte cycle(){
    static byte step = 0;
    step += 4;
    if(step < cpu.cycles){
        return 4;
    }
    const byte cur_instr = read(PC);
    execute(cur_instr);

    PC += pc_inc[cur_instr];
    cpu.cycles = cycles[cur_instr];

    return 4;
}

void print_registers(){
    printf("A:%02X ", A);
    printf("F:%02X ", F);
    printf("B:%02X ", B);
    printf("C:%02X ", C);
    printf("D:%02X ", D);
    printf("E:%02X ", E);
    printf("H:%02X ", H);
    printf("L:%02X ", L);
    printf("SP:%04X ", SP);
    printf("PC:%04X ", PC);
    printf("PCMEM:%02X,%02X,%02X,%02X\n", 
        read(PC), read(PC+1), read(PC+2), read(PC+3));
}

static inline void LD(byte *dst, byte val){
    *dst = val;
}

static inline void LD16(word *dst, word val){
    *dst = val;
}

static inline void LDA8(){
    word addr = read16(PC+1);
    byte low = SP & 0x00FF;
    byte high = SP >> 8;
    write(addr, low); addr++;
    write(addr, high);
}

static inline void INC(byte *dst){
    const byte dst_val = *dst;
    *dst += 1;
    SET_FLAGS(*dst == 0, 
            RESET, 
            ((dst_val & 0x0F) + (1 & 0x0F)) > 0x0F, 
            DO_NOTHING 
    ); 
}

static inline void INC_MEM(word address){
    const byte old_mem_val = read(address);
    write(address, old_mem_val + 1);
    SET_FLAGS(read(address) == 0, 
            RESET, 
            ((old_mem_val & 0x0F) + (1 & 0x0F)) > 0x0F, 
            DO_NOTHING 
    ); 
}

static inline void DEC(byte *dst){
    const byte dst_val = *dst;
    *dst -= 1;
    SET_FLAGS(*dst == 0, 
            SET, 
            !(dst_val & 0xFF), 
            DO_NOTHING 
    ); 
}

static inline void DEC_MEM(word address){
    const byte old_mem_val = read(address);
    write(address, old_mem_val - 1);
    SET_FLAGS(read(address) == 0, 
            SET, 
            !(old_mem_val & 0x0F), 
            DO_NOTHING 
    ); 
}

static inline void RLA(){ 
    const byte carry_val = F & FLAG_C;
    const byte will_carry = A & ( 1 << 7 );
    A <<= 1;
    if(carry_val) A += 1;
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RRA(){
    const byte carry_val = F & FLAG_C;
    const byte will_carry = A & 0x01;
    A >>= 1;
    if(carry_val) A |= (1<<7);
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RLCA(){
    const byte will_carry = A & ( 1 << 7 );
    A <<= 1;
    if(will_carry) A += 1;
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void RRCA(){
    const byte will_carry = A & 0x01;
    A >>= 1;
    if(will_carry) A |= (1<<7);
    SET_FLAGS(RESET, 
            RESET, 
            RESET, 
            will_carry 
    ); 
}

static inline void JRN(byte flag, byte opcode){ //jump if not set
    const byte flag_set = F & flag;
    if(!flag_set){
        PC+= (int)read(PC+1); //need signed data here     
        cycles[opcode] = 12; 
    }else{
        cycles[opcode] = 8;
    }
}

static inline void JRS(byte flag, byte opcode){ //jump if set
    const byte flag_set = F & flag;
    if(flag_set){
        PC+= (int)read(PC+1); //need signed data here     
        cycles[opcode] = 12; 
    }else{
        cycles[opcode] = 8;
    }
}

static inline void JR(byte opcode){
    PC+=(int)read(PC+1);
}

//https://blog.ollien.com/posts/gb-daa/ 
static inline void DAA(){
    byte should_carry = 0;
    byte half_carry = F & FLAG_H;
    byte carry = F & FLAG_C;
    byte subtract = F & FLAG_N;
    
    const byte dst_val = A;
    byte offset = 0;
    if((subtract == 0 && ((dst_val & 0x0F) > 0x09)) || half_carry == 1){
        offset |= 0x06;
    }

    if((subtract == 0 &&  dst_val > 0x99) || carry == 1){
        offset |= 0x60;
        should_carry = 1;
    }

    A = subtract ? dst_val - offset : dst_val + offset;
    SET_FLAGS(A == 0, 
            DO_NOTHING, 
            RESET, 
            (dst_val > 0x99 && subtract == 0) || should_carry  
    ); 
}

static inline void SCF(){
    SET_FLAGS(DO_NOTHING, 
            RESET, 
            RESET, 
            SET  
    );    
}

static inline void CPL(){
    A = ~A;
    SET_FLAGS(DO_NOTHING, 
            SET, 
            SET, 
            DO_NOTHING  
    );  
}

static inline void CCF(){
    const byte carry_set = F & FLAG_C;
    SET_FLAGS(DO_NOTHING, 
            RESET, 
            RESET, 
            !carry_set  
    );  
}

static inline void HALT(){
    cpu.is_halted = 1;
}

static inline void ADD(byte* dst, byte val) {
    const byte dst_val = *dst;
    const word result = dst_val + val;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            RESET, 
            ((dst_val & 0x0F) + (val & 0x0F)) > 0x0F, 
            result > 0xFF
    ); 
}

static inline void ADD16(word* dst, word val) {
    const word dst_val = *dst;
    const int result = dst_val + val;
    *dst = (word)result;
    SET_FLAGS(DO_NOTHING, 
            RESET, 
            ((dst_val & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF, 
            result > 0xFFFF
    ); 
}

static inline void ADC(byte *dst, byte val){
    const byte carry_set = F & FLAG_C;
    const word dst_val = *dst;
    const word result = dst_val + val + carry_set;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            RESET, 
            ((dst_val & 0x0F) + (val & 0x0F) + carry_set) > 0x0F, 
            result > 0xFF
    ); 
}

static inline void SUB(byte *dst, byte val){
    const byte dst_val = *dst;
    const word result = dst_val - val;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            SET, 
            (dst_val & 0x0F) < (val & 0x0F), 
            result > 0xFF
    ); 
}

static inline void SBC(byte *dst, byte val){ 
    const byte carry_set = F & FLAG_C;
    const byte dst_val = *dst;
    const word result = dst_val - val - carry_set;
    *dst = (byte)result;
    SET_FLAGS(*dst == 0, 
            SET, 
            (dst_val & 0x0F) < (val & 0x0F) + carry_set, 
            result > 0xFF
    ); 
}

static inline void AND(byte *dst, byte val){
    *dst &= val;
    SET_FLAGS(*dst == 0, RESET, SET, RESET);
}

static inline void XOR(byte *dst, byte val){
    *dst ^= val;
    SET_FLAGS(*dst == 0, RESET, RESET, RESET);
}

static inline void OR(byte *dst, byte val){
    *dst |= val;
    SET_FLAGS(*dst == 0, RESET, RESET, RESET);
}

static inline void CP(byte *dst, byte val){
    const word result = *dst - val;
    SET_FLAGS(result == 0, 
            SET, 
            (*dst & 0x0F) < (val & 0x0F), 
            result > 0xFF
    ); 
}

static void execute(byte opcode){
    switch(opcode){
        case 0x00: break; //NOP
        case 0x01: LD16(&BC, read16(PC+1)); break;
        case 0x02: write(BC, A); break;
        case 0x03: BC++; break;
        case 0x04: INC(&B); break;
        case 0x05: DEC(&B); break;
        case 0x06: LD(&B, read(PC+1)); break;
        case 0x07: RLCA(); break;
        case 0x08: LDA8(); break;
        case 0x09: ADD16(&HL, BC); break;
        case 0x0A: LD(&A, read(BC)); break;
        case 0x0B: BC--; break;
        case 0x0C: INC(&C); break;
        case 0x0D: DEC(&C); break;
        case 0x0E: LD(&C, read(PC+1)); break;
        case 0x0F: RRCA(); break;

        case 0x10: break; //STOP
        case 0x11: LD16(&DE, read16(PC+1)); break;
        case 0x12: write(DE, A); break;
        case 0x13: DE++; break;
        case 0x14: INC(&D); break;
        case 0x15: DEC(&D); break;
        case 0x16: LD(&D, read(PC+1)); break;
        case 0x17: RLA(); break;
        case 0x18: JR(0x18); break;
        case 0x19: ADD16(&HL, DE); break;
        case 0x1A: LD(&A, read(DE));break;
        case 0x1B: DE--; break;
        case 0x1C: INC(&E); break;
        case 0x1D: DEC(&E); break;
        case 0x1E: LD(&E, read(PC+1)); break;
        case 0x1F: RRA(); break;

        case 0x20: JRN(FLAG_Z, 0x20); break;
        case 0x21: LD16(&HL, read16(PC+1)); break;
        case 0x22: write(HL, A); HL++; break;
        case 0x23: HL++; break;
        case 0x24: INC(&H); break;
        case 0x25: DEC(&H); break;
        case 0x26: LD(&H, read(PC+1)); break;
        case 0x27: DAA(); break;
        case 0x28: JRS(FLAG_Z, 0x28); break;
        case 0x29: ADD16(&HL, HL); break;
        case 0x2A: LD(&A, read(HL)); HL++; break;
        case 0x2B: HL--; break;
        case 0x2C: INC(&L); break;
        case 0x2D: DEC(&L); break;
        case 0x2E: LD(&L, read(PC+1)); break;
        case 0x2F: CPL(); break;

        case 0x30: JRN(FLAG_C, 0x30); break;
        case 0x31: LD16(&SP, read16(PC+1)); break;
        case 0x32: write(HL, A); HL--; break;
        case 0x33: SP++; break;
        case 0x34: INC_MEM(HL); break;
        case 0x35: DEC_MEM(HL); break;
        case 0x36: write(HL, read(PC+1)); break;
        case 0x37: SCF(); break;
        case 0x38: JRS(FLAG_C, 0x38); break;
        case 0x39: ADD16(&HL, SP); break;
        case 0x3A: LD(&A, read(HL)); HL--; break;
        case 0x3B: SP--; break;
        case 0x3C: INC(&A); break;
        case 0x3D: DEC(&A); break;
        case 0x3E: LD(&A, read(PC+1)); break;
        case 0x3F: CCF(); break;

        case 0x40: LD(&B, B); break;
        case 0x41: LD(&B, C); break;
        case 0x42: LD(&B, D); break;
        case 0x43: LD(&B, E); break;
        case 0x44: LD(&B, H); break;
        case 0x45: LD(&B, L); break;
        case 0x46: LD(&B, read(HL)); break;
        case 0x47: LD(&B, A); break;
        case 0x48: LD(&C, B); break;
        case 0x49: LD(&C, C); break;
        case 0x4A: LD(&C, D); break;
        case 0x4B: LD(&C, E); break;
        case 0x4C: LD(&C, H); break;
        case 0x4D: LD(&C, L); break;
        case 0x4E: LD(&C, read(HL)); break;
        case 0x4F: LD(&C, A); break;

        case 0x50: LD(&D, B); break;
        case 0x51: LD(&D, C); break;
        case 0x52: LD(&D, D); break;
        case 0x53: LD(&D, E); break;
        case 0x54: LD(&D, H); break;
        case 0x55: LD(&D, L); break;
        case 0x56: LD(&D, read(HL)); break;
        case 0x57: LD(&D, A); break;
        case 0x58: LD(&E, B); break;
        case 0x59: LD(&E, C); break;
        case 0x5A: LD(&E, D); break;
        case 0x5B: LD(&E, E); break;
        case 0x5C: LD(&E, H); break;
        case 0x5D: LD(&E, L); break;
        case 0x5E: LD(&E, read(HL)); break;
        case 0x5F: LD(&E, A); break;

        case 0x60: LD(&H, B); break;
        case 0x61: LD(&H, C); break;
        case 0x62: LD(&H, D); break;
        case 0x63: LD(&H, E); break;
        case 0x64: LD(&H, H); break;
        case 0x65: LD(&H, L); break;
        case 0x66: LD(&H, read(HL)); break;
        case 0x67: LD(&H, A); break;
        case 0x68: LD(&L, B); break;
        case 0x69: LD(&L, C); break;
        case 0x6A: LD(&L, D); break;
        case 0x6B: LD(&L, E); break;
        case 0x6C: LD(&L, H); break;
        case 0x6D: LD(&L, L); break;
        case 0x6E: LD(&L, read(HL)); break;
        case 0x6F: LD(&L, A); break;

        case 0x70: write(HL, B); break;
        case 0x71: write(HL, C); break;
        case 0x72: write(HL, D); break;
        case 0x73: write(HL, E); break;
        case 0x74: write(HL, H); break;
        case 0x75: write(HL, L); break;
        case 0x76: HALT(); break;
        case 0x77: write(HL, A); break;
        case 0x78: LD(&A, B); break;
        case 0x79: LD(&A, C); break;
        case 0x7A: LD(&A, D); break;
        case 0x7B: LD(&A, E); break;
        case 0x7C: LD(&A, H); break;
        case 0x7D: LD(&A, L); break;
        case 0x7E: LD(&A, read(HL)); break;
        case 0x7F: LD(&A, A); break;
        
        case 0x80: ADD(&A, B); break;
        case 0x81: ADD(&A, C); break;
        case 0x82: ADD(&A, D); break;
        case 0x83: ADD(&A, E); break;
        case 0x84: ADD(&A, H); break;
        case 0x85: ADD(&A, L); break;
        case 0x86: ADD(&A, read(HL)); break;
        case 0x87: ADD(&A, A); break;
        case 0x88: ADC(&A, B); break;
        case 0x89: ADC(&A, C); break;
        case 0x8A: ADC(&A, D); break;
        case 0x8B: ADC(&A, E); break;
        case 0x8C: ADC(&A, H); break;
        case 0x8D: ADC(&A, L); break;
        case 0x8E: ADC(&A, read(HL)); break;
        case 0x8F: ADC(&A, A); break;

        case 0x90: SUB(&A, B); break;
        case 0x91: SUB(&A, C); break;
        case 0x92: SUB(&A, D); break;
        case 0x93: SUB(&A, E); break;
        case 0x94: SUB(&A, H); break;
        case 0x95: SUB(&A, L); break;
        case 0x96: SUB(&A, read(HL)); break;
        case 0x97: SUB(&A, A); break;
        case 0x98: SBC(&A, B); break;
        case 0x99: SBC(&A, C); break;
        case 0x9A: SBC(&A, D); break;
        case 0x9B: SBC(&A, E); break;
        case 0x9C: SBC(&A, H); break;
        case 0x9D: SBC(&A, L); break;
        case 0x9E: SBC(&A, read(HL)); break;
        case 0x9F: SBC(&A, A); break;

        case 0xA0: AND(&A, B); break;
        case 0xA1: AND(&A, C); break;
        case 0xA2: AND(&A, D); break;
        case 0xA3: AND(&A, E); break;
        case 0xA4: AND(&A, H); break;
        case 0xA5: AND(&A, L); break;
        case 0xA6: AND(&A, read(HL)); break;
        case 0xA7: AND(&A, A); break;
        case 0xA8: XOR(&A, B); break;
        case 0xA9: XOR(&A, C); break;
        case 0xAA: XOR(&A, D); break;
        case 0xAB: XOR(&A, E); break;
        case 0xAC: XOR(&A, H); break;
        case 0xAD: XOR(&A, L); break;
        case 0xAE: XOR(&A, read(HL)); break;
        case 0xAF: XOR(&A, A); break;

        case 0xB0: OR(&A, B); break;
        case 0xB1: OR(&A, C); break;
        case 0xB2: OR(&A, D); break;
        case 0xB3: OR(&A, E); break;
        case 0xB4: OR(&A, H); break;
        case 0xB5: OR(&A, L); break;
        case 0xB6: OR(&A, read(HL)); break;
        case 0xB7: OR(&A, A); break;
        case 0xB8: CP(&A, B); break;
        case 0xB9: CP(&A, C); break;
        case 0xBA: CP(&A, D); break;
        case 0xBB: CP(&A, E); break;
        case 0xBC: CP(&A, H); break;
        case 0xBD: CP(&A, L); break;
        case 0xBE: CP(&A, read(HL)); break;
        case 0xBF: CP(&A, A); break;

        case 0xC0: break;
        case 0xC1: break;
        case 0xC2: break;
        case 0xC3: break;
        case 0xC4: break;
        case 0xC5: break;
        case 0xC6: break;
        case 0xC7: break;
        case 0xC8: break;
        case 0xC9: break;
        case 0xCA: break;
        case 0xCB: break;
        case 0xCC: break;
        case 0xCD: break;
        case 0xCE: break;
        case 0xCF: break;

        case 0xD0: break;
        case 0xD1: break;
        case 0xD2: break;
        case 0xD3: break;
        case 0xD4: break;
        case 0xD5: break;
        case 0xD6: break;
        case 0xD7: break;
        case 0xD8: break;
        case 0xD9: break;
        case 0xDA: break;
        case 0xDB: break;
        case 0xDC: break;
        case 0xDD: break;
        case 0xDE: break;
        case 0xDF: break;

        case 0xE0: break;
        case 0xE1: break;
        case 0xE2: break;
        case 0xE3: break;
        case 0xE4: break;
        case 0xE5: break;
        case 0xE6: break;
        case 0xE7: break;
        case 0xE8: break;
        case 0xE9: break;
        case 0xEA: break;
        case 0xEB: break;
        case 0xEC: break;
        case 0xED: break;
        case 0xEE: break;
        case 0xEF: break;

        case 0xF0: break;
        case 0xF1: break;
        case 0xF2: break;
        case 0xF3: break;
        case 0xF4: break;
        case 0xF5: break;
        case 0xF6: break;
        case 0xF7: break;
        case 0xF8: break;
        case 0xF9: break;
        case 0xFA: break;
        case 0xFB: break;
        case 0xFC: break;
        case 0xFD: break;
        case 0xFE: break;
        case 0xFF: break;

        default: break;
    }
}

//sometimes cycles/pc increments can vary but this will cover most cases
static byte cycles[0x100] = {
  /*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F	*/
    4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4,	/* 0x00 */
    4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,	/* 0x10 */
    8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,	/* 0x20 */
    8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4,	/* 0x30 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x40 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x50 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x60 */
    8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x70 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x80 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0x90 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0xA0 */
    4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,	/* 0xB0 */
    8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  8, 12, 24,  8, 16,	/* 0xC0 */
    8, 12, 12,  0, 12, 16,  8, 16,  8, 16, 12,  0, 12,  0,  8, 16,	/* 0xD0 */
   12, 12,  8,  0,  0, 16,  8, 16, 16,  4, 16,  0,  0,  0,  8, 16,	/* 0xE0 */
   12, 12,  8,  4,  0, 16,  8, 16, 12,  8, 16,  4,  0,  0,  8, 16	/* 0xF0 */
};

static const byte pc_inc[0x100] = {
  /*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F	*/
    1,  3,  1,  1,  1,  1,  2,  1,  3,  1,  1,  1,  1,  1,  2,  1,	/* 0x00 */
    1,  2,  3,  1,  1,  1,  2,  1,  2,  1,  1,  1,  1,  1,  2,  1,	/* 0x10 */
    2,  3,  1,  1,  1,  1,  2,  1,  3,  1,  1,  1,  1,  1,  2,  1,	/* 0x20 */
    1,  2,  3,  1,  1,  1,  2,  1,  3,  1,  1,  1,  1,  1,  2,  1,	/* 0x30 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x40 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x50 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x60 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x70 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x80 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0x90 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0xA0 */
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,	/* 0xB0 */
    1,  3,  3,  3,  3,  3,  1,  3,  1,  3,  3,  1,  3,  3,  1,  3,	/* 0xC0 */
    1,  2,  2,  1,  2,  3,  1,  3,  1,  3,  3,  1,  2,  1,  1,  3,	/* 0xD0 */
    2,  3,  2,  1,  1,  3,  1,  3,  3,  1,  3,  1,  1,  1,  1,  3,	/* 0xE0 */
    2,  3,  2,  1,  1,  3,  1,  3,  3,  1,  3,  1,  1,  1,  1,  3	/* 0xF0 */
};

static const byte extended_cycles[0x100] = {
  /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x00 */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x10 */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x20 */
     8,  8,  8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, /* 0x30 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x40 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x50 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x60 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x70 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x80 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0x90 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xA0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xB0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xC0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xD0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, /* 0xE0 */
     8,  8,  8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8  /* 0xF0 */
};

static const byte extended_pc_inc[0x100] = {
  /* 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x00 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x10 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x20 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x30 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x40 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x50 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x60 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x70 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x80 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0x90 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xA0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xB0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xC0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xD0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xE0 */
     2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, /* 0xF0 */
};
