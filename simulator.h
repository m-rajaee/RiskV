#pragma once
#define SIMULATOR_H
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <stdint.h>
#include <stdexcept>
using namespace std;

const uint32_t MEM_SIZE = 1024 * 64;
const uint32_t REG_COUNT = 32;
const uint32_t PROGRAM_START = 0x100;

static const char* reg_names[32] = {
       "zero","ra","sp","gp","tp","t0","t1","t2",
       "s0","s1","a0","a1","a2","a3","a4","a5",
       "a6","a7","s2","s3","s4","s5","s6","s7",
       "s8","s9","s10","s11","t3","t4","t5","t6"
};

class Register
{
    uint32_t value;
public:
    Register();
    uint32_t read() const;
    void write(uint32_t v);
    void reset();
};
class Simulator
{
    uint32_t mem[MEM_SIZE] = { 0 };
    array<Register, REG_COUNT> regfile;
    Register PC, MAR, MDR, IR, A, B, ALUOut;
    int clk;
    void reset_clk();
    void R_type(uint32_t instr);
    void S_type(uint32_t instr);
    void B_type(uint32_t instr);
    void J_type(uint32_t instr);
    void I_type(uint32_t instr, uint32_t opcode);
    void U_type(uint32_t instr, uint32_t opcode);
public:
    Simulator();
    void load_program(const string& path);
    void print_state();
    void start();
    void writeWord(uint32_t input, uint32_t address);
    void writeHalf(uint16_t input, uint32_t address);
    void writeByte(uint8_t input, uint32_t address);
};

