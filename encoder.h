#pragma once
#ifndef ENCODER_H
#define ENCODER_H
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <stdexcept>
using namespace std;
class SymbolTable {
public:
    unordered_map<string, uint32_t> table;
    SymbolTable() = default;
    void addLabel(const string& label, uint32_t address);
    bool hasLabel(const string& label);
    uint32_t getAddress(const string& label) const;
};

uint32_t encodeInstruction(const vector<string>& tokens, uint32_t address, const SymbolTable& sym, unordered_map<string, uint32_t> regMap);

#endif