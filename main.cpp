﻿#include "encoder.h"
#include "simulator.h"
using namespace std;

// === Utility Functions ===
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}
string remove_commas(const string& s) {
    string result;
    for (char c : s) if (c != ',') result += c;
    return result;
}
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream ss(s);
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}
pair<string, string> parseMemoryOperand(const string& operand) {
    size_t open = operand.find('(');
    size_t close = operand.find(')');
    if (open == string::npos || close == string::npos || close <= open)
        throw runtime_error("Invalid memory operand: " + operand);
    string imm = operand.substr(0, open);
    string reg = operand.substr(open + 1, close - open - 1);
    return { trim(imm), trim(reg) };
}
unordered_map<string, uint32_t> regMap = {
    {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3}, {"x4", 4}, {"x5", 5},
    {"x6", 6}, {"x7", 7}, {"x8", 8}, {"x9", 9}, {"x10", 10}, {"x11", 11},
    {"x12", 12}, {"x13", 13}, {"x14", 14}, {"x15", 15}, {"x16", 16}, {"x17", 17},
    {"x18", 18}, {"x19", 19}, {"x20", 20}, {"x21", 21}, {"x22", 22}, {"x23", 23},
    {"x24", 24}, {"x25", 25}, {"x26", 26}, {"x27", 27}, {"x28", 28}, {"x29", 29},
    {"x30", 30}, {"x31", 31}
};

int main() {
    SymbolTable symbolTable;
    vector<string> instructions;
    uint32_t address = 0x1000;
    ifstream infile("input.asm");
    string line;
    Simulator simulator;

    // ─────[ Pass 1: Label Parsing and Directives ]─────
    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') 
            continue;

        if (line.find(':') != string::npos) {
            string label = trim(line.substr(0, line.find(':')));
            symbolTable.addLabel(label, address);
            line = trim(line.substr(line.find(':') + 1));
            if (line.empty()) 
                continue;
        }

        if (line[0] == '.') {
            vector<string> tokens = split(line, ' ');
            string directive = tokens[0];

            if (directive == ".org") {
                address = stoul(tokens[1], nullptr, 0);
            }
            else if (directive == ".word") {
                simulator.writeWord(stoul(tokens[1], nullptr, 0), address);
                address += 4;
            }
            else if (directive == ".half") {
                simulator.writeHalf((uint16_t)stoul(tokens[1], nullptr, 0), address);
                address += 2;
            }
            else if (directive == ".byte") {
                simulator.writeByte((uint8_t)stoul(tokens[1], nullptr, 0), address);
                address += 1;
            }
            else if (directive == ".align") {
                int n = stoul(tokens[1]);
                uint32_t alignTo = 1 << n;
                address = (address + alignTo - 1) & ~(alignTo - 1);
            }
            continue;
        }

        instructions.push_back(line);
        address += 4;
    }

    // ─────[ Pass 2: Instruction Encoding ]─────
    ofstream outfile("output.txt");
    address = 0x1000;
    ifstream infile2("input.asm");
    string instr;
    string working_string;
    while (getline(infile2, instr)) {
        working_string = trim(instr);
        if (working_string.empty() || working_string[0] == '#') 
            continue;
        if (working_string.find(':') != string::npos) {
            working_string = trim(working_string.substr(working_string.find(':') + 1));
            if (working_string.empty()) 
                continue;
        }

        if (working_string[0] == '.') {
            vector<string> tokens = split(working_string, ' ');
            string directive = tokens[0];
            if (directive == ".org") {
                address = stoul(tokens[1], nullptr, 0);
            }
            else if (directive == ".word" || directive == ".half" || directive == ".byte") {
                if (directive == ".word") 
                    address += 4;
                else if (directive == ".half") 
                    address += 2;
                else if (directive == ".byte") 
                    address += 1;
            }
            else if (directive == ".align") {
                int n = stoi(tokens[1]);
                uint32_t alignTo = 1 << n;
                address = (address + alignTo - 1) & ~(alignTo - 1);
            }
            continue;
        }
        string inputline = remove_commas(trim(instr));
        vector<string> tokens = split(inputline, ' ');
        if (tokens[0] == "li") {
            uint32_t imm = stoul(tokens[2], nullptr, 0);
            if (imm <= 0xFFF) {
                vector<string> newtokens = { "addi",tokens[1],"x0",tokens[2]};
                uint32_t code = encodeInstruction(newtokens, address, symbolTable, regMap);
                outfile << hex << setw(8) << setfill('0') << code << endl;
                continue;
            }
            uint32_t upper = imm >> 12;
            uint32_t lower = imm & 0xFFF;
            vector<string> newtokens = { "lui",tokens[1],to_string(upper)};
            uint32_t code = encodeInstruction(newtokens, address, symbolTable, regMap);
            outfile << hex << setw(8) << setfill('0') << code << endl;
            vector<string> newtokens2 = { "addi",tokens[1],tokens[1],to_string(lower)};
            code = encodeInstruction(newtokens2, address, symbolTable, regMap);
            outfile << hex << setw(8) << setfill('0') << code << endl;
            continue;
        }
        if (tokens.size() == 3 && tokens[2].find('(') != string::npos) {
            pair<string, string> temp = parseMemoryOperand(tokens[2]);
            tokens.pop_back();
            tokens.push_back(temp.second);
            tokens.push_back(temp.first);
        }
;       uint32_t code = encodeInstruction(tokens, address, symbolTable, regMap);
        outfile << hex << setw(8) << setfill('0') << code << endl;
        simulator.writeWord(code, address);
        address += 4;
    }
    infile2.close();
    outfile.close();

    // ─────[ Pass 3: Simulation ]─────
    simulator.load_program("output.txt");
    simulator.start();
}