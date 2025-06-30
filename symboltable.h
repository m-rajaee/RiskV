#pragma once
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