#include "encoder.h"
#include "simulator.h"
#include <algorithm>
// === Utility Functions ===
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}
string remove_commas(const string& s) {
    string result;
    for (char c : s) {
        if (c != ',') result += c;
    }
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
unordered_map<string, uint32_t> regMap = {
    {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3}, {"x4", 4},
    {"x5", 5}, {"x6", 6}, {"x7", 7}, {"x8", 8}, {"x9", 9},
    {"x10", 10}, {"x11", 11}, {"x12", 12}, {"x13", 13}, {"x14", 14},
    {"x15", 15}, {"x16", 16}, {"x17", 17}, {"x18", 18}, {"x19", 19},
    {"x20", 20}, {"x21", 21}, {"x22", 22}, {"x23", 23}, {"x24", 24},
    {"x25", 25}, {"x26", 26}, {"x27", 27}, {"x28", 28}, {"x29", 29},
    {"x30", 30}, {"x31", 31}
};
int main() {
    SymbolTable symbolTable;
    vector<string> instructions;
    uint32_t address = 0x1000;
    ifstream infile("input.asm");
    string line;
    Simulator simulator;
    // ───────────────[ PAS 1: PARSER ]───────────────
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
        else if (line[0] == '.') {         // Directive instructions
            vector<string> tokens = split(line, ' ');
            string directive = tokens[0];
            if (directive == ".org") { // set address of program to given value
                address = stoul(tokens[1], nullptr, 0);
            }
            else if (directive == ".word") {  //put a 32 bit value to memory
                uint32_t input = stoul(tokens[1], nullptr, 0);
                simulator.writeWord(input,address);
                address += 4;
            }
            else if (directive == ".half") {  //put a 16 bit value to memory
                uint16_t input = stoul(tokens[1], nullptr, 0);
                simulator.writeHalf(input, address);
                address += 2;
            }
            else if (directive == ".byte") {  //put a 8 bit value to memory
                uint8_t input = stoul(tokens[1], nullptr, 0);
                simulator.writeByte(input, address);
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
    // ───────────────[ PAS 2: ENCODER ]───────────────
    ofstream outfile("output.txt", ios::binary);
    address = 0x1000;
    for (const string& instr : instructions) {
        string inputline = trim(instr);
        inputline = remove_commas(inputline);
        vector<string> tokens = split(inputline, ' ');
        uint32_t code = encodeInstruction(tokens, address, symbolTable, regMap);
        outfile.write(reinterpret_cast<char*>(&code), sizeof(code));
        address += 4;
    }
    outfile.close();
    // ───────────────[ PAS 3: SIMULATOR ]───────────────
    simulator.load_program("output.txt");
    simulator.start();
}