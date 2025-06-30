#include "encoder.h"

// === Utility Functions ===
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
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
unordered_map<string, uint8_t> regMap = {
    {"x0", 0}, {"x1", 1}, {"x2", 2}, {"x3", 3}, {"x4", 4},
    {"x5", 5}, {"x6", 6}, {"x7", 7}, {"x8", 8}, {"x9", 9},
    {"x10", 10}, {"x11", 11}, {"x12", 12}, {"x13", 13}, {"x14", 14},
    {"x15", 15}, {"x16", 16}, {"x17", 17}, {"x18", 18}, {"x19", 19},
    {"x20", 20}, {"x21", 21}, {"x22", 22}, {"x23", 23}, {"x24", 24},
    {"x25", 25}, {"x26", 26}, {"x27", 27}, {"x28", 28}, {"x29", 29},
    {"x30", 30}, {"x31", 31}
};
int main() {
    SymbolTable sym;
    vector<string> instructions;
    uint32_t address = 0x1000;
    ifstream infile("input.asm");
    string line;
    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') 
            continue;
        if (line.find(':') != string::npos) {
            string label = trim(line.substr(0, line.find(':')));
            sym.addLabel(label, address);
            line = trim(line.substr(line.find(':') + 1));
            if (line.empty()) 
                continue;
        }
        else if (line[0] == '.') {
            vector<string> tokens = split(line, ' ');
            string directive = tokens[0];
            if (directive == ".org") {
                address = stoi(tokens[1], nullptr, 0);
                continue;
            }
            else if (directive == ".word") {
                //put a 32 bit value to memory
            }
            else if (directive == ".half") {
                //put a 16 bit value to memory
            }
            else if (directive == ".byte") {
                //put a 8 bit value to memory
            }
            else if (directive == ".align") {
                int n = stoi(tokens[1]);
                uint32_t alignTo = 1 << n;
                address = (address + alignTo - 1) & ~(alignTo - 1);
                continue;
            }
        }
        instructions.push_back(line);
        address += 4;
    }
}