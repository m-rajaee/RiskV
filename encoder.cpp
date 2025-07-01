#include "encoder.h"
void SymbolTable::addLabel(const string& label, uint32_t address){ table[label] = address; }
bool SymbolTable::hasLabel(const string& label){ return table.find(label) != table.end(); }
uint32_t SymbolTable::getAddress(const string& label) const {
    auto it = table.find(label);
    return (it != table.end()) ? it->second : 0;
}
uint32_t encodeInstruction(const vector<string>& tokens, uint32_t address, const SymbolTable& sym, unordered_map<string, uint32_t> regMap) {
    string inst = tokens[0];
    uint32_t rd, rs1, rs2, imm, opcode, funct3, funct7;

    if (inst == "add" || inst == "sub" || inst == "xor" || inst == "or" || inst == "and" ||
        inst == "sll" || inst == "srl" || inst == "sra" || inst == "slt" || inst == "sltu") {
        rd = regMap[tokens[1]];
        rs1 = regMap[tokens[2]];
        rs2 = regMap[tokens[3]];
        opcode = 0b0110011;

        if (inst == "add") { funct3 = 0b000; funct7 = 0b0000000; }
        else if (inst == "sub") { funct3 = 0b000; funct7 = 0b0100000; }
        else if (inst == "xor") { funct3 = 0b100; funct7 = 0b0000000; }
        else if (inst == "or") { funct3 = 0b110; funct7 = 0b0000000; }
        else if (inst == "and") { funct3 = 0b111; funct7 = 0b0000000; }
        else if (inst == "sll") { funct3 = 0b001; funct7 = 0b0000000; }
        else if (inst == "srl") { funct3 = 0b101; funct7 = 0b0000000; }
        else if (inst == "sra") { funct3 = 0b101; funct7 = 0b0100000; }
        else if (inst == "slt") { funct3 = 0b010; funct7 = 0b0000000; }
        else if (inst == "sltu") { funct3 = 0b011; funct7 = 0b0000000; }

        return (funct7 << 25) | (rs2 << 20) | (rs1 << 15)
            | (funct3 << 12) | (rd << 7) | opcode;
    }

    // Immediate type (I-type)
    if (inst == "addi" || inst == "xori" || inst == "ori" || inst == "andi" ||
        inst == "slli" || inst == "srli" || inst == "srai" ||
        inst == "slti" || inst == "sltiu" || inst == "jalr" ||
        inst == "lb" || inst == "lh" || inst == "lw" || inst == "lbu" || inst == "lhu") {
        rd = regMap[tokens[1]];
        rs1 = regMap[tokens[2]];
        imm = stoul(tokens[3]);
        if (inst == "addi") { opcode = 0b0010011; funct3 = 0b000; }
        else if (inst == "xori") { opcode = 0b0010011; funct3 = 0b100; }
        else if (inst == "ori") { opcode = 0b0010011; funct3 = 0b110; }
        else if (inst == "andi") { opcode = 0b0010011; funct3 = 0b111; }
        else if (inst == "slli") { opcode = 0b0010011; funct3 = 0b001; funct7 = 0b0000000; return (funct7 << 25) | ((imm & 0x1F) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode; }
        else if (inst == "srli") { opcode = 0b0010011; funct3 = 0b101; funct7 = 0b0000000; return (funct7 << 25) | ((imm & 0x1F) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode; }
        else if (inst == "srai") { opcode = 0b0010011; funct3 = 0b101; funct7 = 0b0100000; return (funct7 << 25) | ((imm & 0x1F) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode; }
        else if (inst == "slti") { opcode = 0b0010011; funct3 = 0b010; }
        else if (inst == "sltiu") { opcode = 0b0010011; funct3 = 0b011; }
        else if (inst == "jalr") { opcode = 0b1100111; funct3 = 0b000; }
        else if (inst == "lb") { opcode = 0b0000011; funct3 = 0b000; }
        else if (inst == "lh") { opcode = 0b0000011; funct3 = 0b001; }
        else if (inst == "lw") { opcode = 0b0000011; funct3 = 0b010; }
        else if (inst == "lbu") { opcode = 0b0000011; funct3 = 0b100; }
        else if (inst == "lhu") { opcode = 0b0000011; funct3 = 0b101; }
        return ((imm & 0xFFF) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    }

    // Store (S-type)
    if (inst == "sb" || inst == "sh" || inst == "sw") {
        rs2 = regMap[tokens[1]];
        rs1 = regMap[tokens[2]];
        imm = stoul(tokens[3]);
        opcode = 0b0100011;
        if (inst == "sb") funct3 = 0b000;
        else if (inst == "sh") funct3 = 0b001;
        else if (inst == "sw") funct3 = 0b010;
        uint32_t imm_11_5 = (imm >> 5) & 0x7F;
        uint32_t imm_4_0 = imm & 0x1F;
        return (imm_11_5 << 25) | (rs2 << 20) | (rs1 << 15)
            | (funct3 << 12) | (imm_4_0 << 7) | opcode;
    }

    // Branch (B-type)
    if (inst == "beq" || inst == "bne" || inst == "blt" || inst == "bge" || inst == "bltu" || inst == "bgeu") {
        rs1 = regMap[tokens[1]];
        rs2 = regMap[tokens[2]];
        int32_t offset = (int32_t)sym.getAddress(tokens[3]) - (int32_t)address;
        opcode = 0b1100011;
        if (inst == "beq") funct3 = 0b000;
        else if (inst == "bne") funct3 = 0b001;
        else if (inst == "blt") funct3 = 0b100;
        else if (inst == "bge") funct3 = 0b101;
        else if (inst == "bltu") funct3 = 0b110;
        else if (inst == "bgeu") funct3 = 0b111;
        uint32_t imm = offset;
        uint32_t imm12 = (imm >> 12) & 1;
        uint32_t imm10_5 = (imm >> 5) & 0x3F;
        uint32_t imm4_1 = (imm >> 1) & 0xF;
        uint32_t imm11 = (imm >> 11) & 1;

        return (imm12 << 31) | (imm11 << 7) | (imm10_5 << 25) | (imm4_1 << 8)
            | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | opcode;
    }

    // J-type (jal)
    if (inst == "jal") {
        rd = regMap[tokens[1]];
        int32_t offset = (int32_t)sym.getAddress(tokens[2]) - (int32_t)address;
        opcode = 0b1101111;
        uint32_t imm = offset;
        uint32_t imm20 = (imm >> 20) & 1;
        uint32_t imm10_1 = (imm >> 1) & 0x3FF;
        uint32_t imm11 = (imm >> 11) & 1;
        uint32_t imm19_12 = (imm >> 12) & 0xFF;
        return (imm20 << 31) | (imm19_12 << 12) | (imm11 << 20) | (imm10_1 << 21) | (rd << 7) | opcode;
    }

    // U-type (lui, auipc)
    if (inst == "lui" || inst == "auipc") {
        rd = regMap[tokens[1]];
        imm = stoul(tokens[2]);
        opcode = (inst == "lui") ? 0b0110111 : 0b0010111;
        return (imm << 12) | (rd << 7) | opcode;
    }

    // Environment (ecall, ebreak)
    if (inst == "ecall") {
        return 0b00000000000000000000000001110011;
    }
    if (inst == "ebreak") {
        return 0b00000000000100000000000001110011;
    }

    throw runtime_error("Unknown instruction: " + inst);
}
