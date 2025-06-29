// RISC-V Assembler + Simulator Project - Simplified (All in main.cpp + headers only where needed)

// ===== main.cpp =====
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
using namespace std;

// hazf tab ezafi aval va akhare har khat
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}
// khat haro be chand string taghsim mikone bar asase char delimiter
vector<string> split(const string& s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream ss(s);
    while (getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    return tokens;
}

// === Symbol Table ===
// label va adderess ha dar in jadval zakhire mishe
class SymbolTable {
    unordered_map<string, uint32_t> table;
public:
    void addLabel(const string& label, uint32_t address) { table[label] = address; }
    bool hasLabel(const string& label) const { return table.find(label) != table.end(); }
    uint32_t getAddress(const string& label) const {
        auto it = table.find(label);
        return (it != table.end()) ? it->second : 0;
    }
};

// tabdil dastoor be code binary
uint32_t encodeInstruction(const vector<string>& tokens, uint32_t address, const SymbolTable& sym) {
    throw runtime_error("Encoding not implemented yet.");
}

// === Simulator ===
class Simulator {
    uint32_t PC = 0x1000;
    uint32_t registers[32] = { 0 };
    unordered_map<uint32_t, uint32_t> memory;
public:
    void loadBinary(const string& filename) {
        ifstream in(filename, ios::binary);
        uint32_t addr = PC, inst;
        while (in.read(reinterpret_cast<char*>(&inst), sizeof(inst))) {
            memory[addr] = inst;
            addr += 4;
        }
    }
    void run() {
        while (memory.find(PC) != memory.end()) {
            uint32_t instr = memory[PC];
            cout << "PC: 0x" << hex << PC << "\tInstr: 0x" << instr << endl;
            PC += 4; // Placeholder for real execution
        }
    }
};

// === Main Function ===
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./assembler <input.asm>\n";
        return 1;
    }

    ifstream infile(argv[1]); // khodane file .asm
    vector<string> lines; // zakhire khat haye dastoor
    SymbolTable sym;
    string line;
    uint32_t address = 0x1000;
    // dar marhale 1 miad labela ro dar jadval save mikone
    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.find(":") != string::npos) {
            string label = trim(line.substr(0, line.find(":")));
            sym.addLabel(label, address);
            line = trim(line.substr(line.find(":")));
            if (line.empty()) continue;
        }
        lines.push_back(line);
        address += 4;
    }
    infile.close();
    address = 0x1000;
    //tabdil dastoorat be binary va zakhire dar output.bin
    ofstream outfile("output.bin", ios::binary);
    for (const auto& l : lines) {
        vector<string> tokens = split(l, ' ');
        uint32_t binary = encodeInstruction(tokens, address, sym);
        outfile.write(reinterpret_cast<char*>(&binary), sizeof(binary));
        address += 4;
    }
    outfile.close();
    Simulator sim; //ejraye shabih saz
    sim.loadBinary("output.bin"); //khondan az output.bin
    sim.run(); // ejraye shabih saz
}
