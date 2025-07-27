#ifndef ALU_H
#define ALU_H

#include <stdint.h>

// Arithmeticv operations 

void ADD_A(uint8_t val);
void ADC_A(uint8_t val);
void SUB_A(uint8_t val);
void SBC_A(uint8_t val);
void CP_A(uint8_t val);
uint8_t INC(uint8_t val);
uint8_t DEC(uint8_t val);


// 16- bit arithmetic ops

void ADD_HL(uint16_t val);
void ADD_SP(uint16_t val);
void INC_16(uint16_t *regist);
void DEC_16(uint16_t *regist);


// Logical Operations

void AND_A(uint8_t val);
void OR_A(uint8_t val);
void XOR_A(uint8_t val);

// misc ALU ops

uint8_t SWAP(uint8_t val);
void DAA();
void CPL();
void CCF();
void SCF();

// rotates and shifts

uint8_t RLC(uint8_t value, bool *carry_out); // rotate n left
uint8_t RL(uint8_t value, bool carry_in, bool *carry_out); // rotate n left through carry
uint8_t RRC(uint8_t value); // rotate n right 
uint8_t RR(uint8_t value); // rotate n right through carry

void SLA(uint8_t *val); // shift n left by 1
void SRA(uint8_t *val); // shift n arithmetic right
void SRL(uint8_t *val); // logical shift right

#endif