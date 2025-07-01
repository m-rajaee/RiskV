#include "simulator.h"
Register::Register() : value(0) {}

void Register::write(uint32_t _v)
{
    value = _v;
}

uint32_t Register::read() const
{
    return value;
}

void Register::reset()
{
    value = 0;
}

Simulator::Simulator()
{
    for (auto& r : regfile)
        r.reset();
    PC.write(PROGRAM_START);
    MAR.write(0);
    MDR.write(0);
    IR.write(0);
    A.write(0);
    B.write(0);
    ALUOut.write(0);
    for (auto& m : mem)
        m = 0;
}

void Simulator::load_program(const string& path)
{
    ifstream infile(path);
    if (!infile)
        throw runtime_error("Cannot open file: " + path);
    string line;
    uint32_t addr = PROGRAM_START / 4;
    while (getline(infile, line))
    {
        if (line.empty())
            continue;
        if (addr >= MEM_SIZE)
            throw runtime_error("Program is too large for memory");
        mem[addr++] = stoul(line, nullptr, 16);
    }
}

void Simulator::print_state()
{
    cout << "clock: " << clk << endl;
    for (int i = 0; i < 32; i += 4)
    {
        cout << hex << setw(3) << setfill(' ') << reg_names[i] << ": "
            << setw(8) << setfill('0') << regfile[i].read() << "  "
            << setw(3) << reg_names[i + 1] << ": "
            << setw(8) << regfile[i + 1].read() << "  "
            << setw(3) << reg_names[i + 2] << ": "
            << setw(8) << regfile[i + 2].read() << "  "
            << setw(3) << reg_names[i + 3] << ": "
            << setw(8) << regfile[i + 3].read() << endl;
    }
    cout << " PC: " << setw(8) << PC.read()
        << "  MAR: " << setw(8) << MAR.read()
        << "  MDR: " << setw(8) << MDR.read()
        << "  IR: " << setw(8) << IR.read()
        << "  A: " << setw(8) << A.read()
        << "  B: " << setw(8) << B.read()
        << "  ALUOut: " << setw(8) << ALUOut.read()
        << dec << endl;
}

void Simulator::start()
{
    clk = 0;
    bool halted = false;
    while (!halted)
    {
        // Cycle 1: MAR ← PC
        clk++;
        MAR.write(PC.read());
        print_state();

        // Cycle 2: MDR ← Mem[MAR]; PC ← PC + 4
        clk++;
        MDR.write(mem[MAR.read() / 4]);
        PC.write(PC.read() + 4);
        print_state();

        // Cycle 3: IR ← MDR
        clk++;
        IR.write(MDR.read());
        print_state();

        // Decode
        uint32_t instr = IR.read();
        uint32_t opcode = instr & 0x7F;

        // Halt if EBREAK encountered
        if (instr == 0x00100073) break;

        switch (opcode)
        {
        case 0x33: // R-type
            R_type(instr);
            break;

        case 0x13: // I-type Arithmetic/Logical/Shift Imm (e.g., addi, slli)
        case 0x03: // I-type Loads (lb, lh, lw, lbu, lhu)
        case 0x67: // I-type Jump (jalr)
            I_type(instr, opcode);
            break;

        case 0x23: // S-type Store Instructions (sh, sw)
            S_type(instr);
            break;

        case 0x37:  // U-type: LUI
        case 0x17:  // U-type: AUIPC
            U_type(instr, opcode);
            break;

        case 0x63:  // B-type branch instructions
            B_type(instr);
            break;

        case 0x6F:  // JAL
            J_type(instr);
            break;

        case 0x73:
            // For system instructions, only EBREAK is implemented.
            halted = true;
            break;

        default:
            halted = true;
            break;
        }
    }
}

void Simulator::reset_clk()
{
    clk = 0;
}

void Simulator::R_type(uint32_t instr)
{
    int rd = (instr >> 7) & 0x1F;
    int funct3 = (instr >> 12) & 0x7;
    int rs1 = (instr >> 15) & 0x1F;
    int rs2 = (instr >> 20) & 0x1F;
    int funct7 = (instr >> 25) & 0x7F;

    // Cycle 4: Read registers into A and B
    clk++;
    A.write(regfile[rs1].read());
    B.write(regfile[rs2].read());
    print_state();

    // Cycle 5: Compute ALU result (R-type operation)
    clk++;
    int32_t val = 0;
    switch (funct3)
    {
    case 0x0:
        if (funct7 == 0x00) val = A.read() + B.read();
        else if (funct7 == 0x20) val = A.read() - B.read();
        else if (funct7 == 0x01) val = int32_t(A.read()) * int32_t(B.read());
        break;
    case 0x1:
        if (funct7 == 0x00) val = A.read() << (B.read() & 0x1F);
        else if (funct7 == 0x01)
        {
            int64_t mulres = int64_t(int32_t(A.read())) * int64_t(int32_t(B.read()));
            val = int32_t(mulres >> 32);
        }
        break;
    case 0x2:
        if (funct7 == 0x00) val = int32_t(A.read()) < int32_t(B.read());
        break;
    case 0x3:
        if (funct7 == 0x00) val = uint32_t(A.read()) < uint32_t(B.read());
        break;
    case 0x4:
        if (funct7 == 0x00) val = A.read() ^ B.read();
        else if (funct7 == 0x01)
        {
            if (B.read() == 0) val = -1;
            else val = int32_t(A.read()) / int32_t(B.read());
        }
        break;
    case 0x5:
        if (funct7 == 0x00) val = uint32_t(A.read()) >> (B.read() & 0x1F);
        else if (funct7 == 0x20) val = int32_t(A.read()) >> (B.read() & 0x1F);
        break;
    case 0x6:
        if (funct7 == 0x00) val = A.read() | B.read();
        else if (funct7 == 0x01)
        {
            if (B.read() == 0) val = A.read();
            else val = int32_t(A.read()) % int32_t(B.read());
        }
        break;
    case 0x7:
        if (funct7 == 0x00) val = A.read() & B.read();
        break;
    }
    ALUOut.write(val);
    print_state();
    // Cycle 6: Write result into rd (if rd != 0)
    clk++;
    if (rd != 0)
        regfile[rd].write(ALUOut.read());
    print_state();

    reset_clk();
}

void Simulator::I_type(uint32_t instr, uint32_t opcode)
{
    int rd = (instr >> 7) & 0x1F;
    int funct3 = (instr >> 12) & 0x7;
    int rs1 = (instr >> 15) & 0x1F;
    int32_t imm = int32_t(instr) >> 20;

    switch (opcode)
    {
    case 0x13: // addi (I-type arithmetic)
        clk++;
        A.write(regfile[rs1].read());
        B.write(imm);
        print_state();

        clk++;
        ALUOut.write(A.read() + B.read());
        print_state();

        clk++;
        if (rd != 0)
            regfile[rd].write(ALUOut.read());
        print_state();
        break;

    case 0x03: // Loads: lh, lw
        clk++;
        A.write(regfile[rs1].read());
        B.write(imm);
        print_state();

        clk++;
        ALUOut.write(A.read() + B.read());
        print_state();

        clk++;
        MAR.write(ALUOut.read());
        print_state();

        clk++;
        {
            uint32_t addrw = (MAR.read() & ~0x3) / 4;
            uint32_t dataw = (addrw < MEM_SIZE) ? mem[addrw] : 0;
            uint32_t addrh = (MAR.read() & ~0x1) / 4;
            uint32_t datah = (addrh < MEM_SIZE) ? mem[addrh] : 0;
            int offseth = (MAR.read() & 0x2) ? 16 : 0;
            int16_t valh = (datah >> offseth) & 0xFFFF;
            switch (funct3)
            {
            case 0x1: // lh
                MDR.write(int32_t(valh));
                break;
            case 0x2: // lw
                MDR.write(int32_t(dataw));
                break;
            }
        }
        print_state();

        clk++;
        if (rd != 0)
            regfile[rd].write(MDR.read());
        print_state();
        break;

    case 0x67: // jalr
        clk++;
        A.write(regfile[rs1].read());
        B.write(imm);
        print_state();

        clk++;
        ALUOut.write(A.read() + B.read());
        print_state();

        clk++;
        if (rd != 0)
            regfile[rd].write(PC.read());
    
        PC.write(ALUOut.read() & ~1u);
        print_state();
        break;
    }
    reset_clk();
}

void Simulator::S_type(uint32_t instr)
{
    int funct3 = (instr >> 12) & 0x7;
    int rs1 = (instr >> 15) & 0x1F;
    int rs2 = (instr >> 20) & 0x1F;
    int32_t imm = ((instr >> 25) << 5) | ((instr >> 7) & 0x1F);
    imm = (imm << 20) >> 20;  // Sign-extend immediate
    // Cycle 4: Read base register into A
    clk++;
    A.write(regfile[rs1].read());
    B.write(imm);
    print_state();

    // Cycle 5: Compute effective address ALUOut ← A + imm
    clk++;
    ALUOut.write(A.read() + B.read());
    print_state();

    // Cycle 6: Set MAR to the effective address
    clk++;
    MAR.write(ALUOut.read());
    print_state();

    // Cycle 7: Get the data to store from rs2 into MDR
    clk++;
    MDR.write(regfile[rs2].read());
    print_state();
    // Cycle 8: Write data from MDR into memory (sw or sh)
    clk++;
    {
        uint32_t addrWord = (MAR.read() & ~0x3u) / 4;
        if (addrWord < MEM_SIZE)
        {
            uint32_t curr = mem[addrWord];
            switch (funct3)
            {
            case 0x1:  // sh: store halfword
            {
                uint16_t half = uint16_t(MDR.read() & 0xFFFF);
                if (MAR.read() & 0x2)
                { // upper half
                    curr = (curr & 0x0000FFFF) | (uint32_t(half) << 16);
                }
                else
                { // lower half
                    curr = (curr & 0xFFFF0000) | uint32_t(half);
                }
                mem[addrWord] = curr;
                break;
            }
            case 0x2:  // sw: store word
                mem[addrWord] = MDR.read();
                break;
            }
        }
    }
    print_state();
    reset_clk();
}

void Simulator::U_type(uint32_t instr, uint32_t opcode)
{
    int rd = (instr >> 7) & 0x1F;
    // Bits[31:12] form the U-immediate; lower 12 bits are zero
    int32_t imm = int32_t(instr & 0xFFFFF000);

    if (opcode == 0x37)  // LUI
    {
        //Cycle 4: B <- imm
        clk++;
        B.write(imm);
        print_state();

        // Cycle 5: ALUOut ← imm << 0   (already shifted in mask)
        clk++;
        ALUOut.write(B.read());
        print_state();

        // Cycle 6: RegFile[rd] ← ALUOut
        clk++;
        if (rd != 0) regfile[rd].write(ALUOut.read());
        print_state();
    }
    else  // AUIPC (0x17)
    {
        // Cycle 4: A ← PC
        clk++;
        A.write(PC.read());
        B.write(imm);
        print_state();

        // Cycle 5: ALUOut ← A + imm
        clk++;
        ALUOut.write(A.read() + B.read());
        print_state();

        // Cycle 6: RegFile[rd] ← ALUOut
        clk++;
        if (rd != 0) regfile[rd].write(ALUOut.read());
        print_state();
    }

    reset_clk();
}

void Simulator::B_type(uint32_t instr)
{
    
    int rs1 = (instr >> 15) & 0x1F;
    int rs2 = (instr >> 20) & 0x1F;
    int funct3 = (instr >> 12) & 0x7;

    int32_t imm = 0;
    imm |= ((instr >> 31) & 0x1) << 12;             // imm[12]
    imm |= ((instr >> 7) & 0x1) << 11;               // imm[11]
    imm |= ((instr >> 25) & 0x3F) << 5;               // imm[10:5]
    imm |= ((instr >> 8) & 0xF) << 1;                // imm[4:1]
    // Sign-extension from 13 bits to 32 bits:
    if (imm & (1 << 12)) {
        imm |= 0xFFFFE000;
    }

    // Cycle 4: Read registers
    clk++;
    A.write(regfile[rs1].read());
    B.write(regfile[rs2].read());
    print_state();

    // Cycle 5: Evaluate branch condition and compute branch target.
    clk++;
    bool takeBranch = false;
    switch (funct3)
    {
    case 0x0: // BEQ: branch if equal
        takeBranch = (A.read() == B.read());
        break;
    case 0x1: // BNE: branch if not equal
        takeBranch = (A.read() != B.read());
        break;
    case 0x4: // BLT: branch if less than (signed)
        takeBranch = (int32_t(A.read()) < int32_t(B.read()));
        break;
    case 0x5: // BGE: branch if greater than or equal (signed)
        takeBranch = (int32_t(A.read()) >= int32_t(B.read()));
        break;
    case 0x6: // BLTU: branch if less than (unsigned)
        takeBranch = (A.read() < B.read());
        break;
    case 0x7: // BGEU: branch if greater than or equal (unsigned)
        takeBranch = (A.read() >= B.read());
        break;
    default:
        // Undefined branch condition: default is no branch.
        break;
    }
    // Compute branch target address.
    // Note: PC currently points to next instruction (PC = old PC + 4 from fetch).
    // Therefore, branch target = (PC - 4) + immediate.
    int32_t branchTarget = (PC.read() - 4) + imm;
    ALUOut.write(branchTarget); // For debug tracking
    print_state();

    // Cycle 6: If branch condition met, update PC with branch target.
    clk++;
    if (takeBranch)
    {
        PC.write(ALUOut.read());
    }
    print_state();

    // Reset the clock counter for the next instruction's cycles.
    reset_clk();
}

void Simulator::J_type(uint32_t instr)
{
    int rd = (instr >> 7) & 0x1F;

    // build 21-bit signed immediate from bits [31], [19:12], [20], [30:21]
    int32_t imm = 0;
    imm |= ((instr >> 31) & 0x1) << 20;       // imm[20]
    imm |= ((instr >> 21) & 0x3FF) << 1;      // imm[10:1]
    imm |= ((instr >> 20) & 0x1) << 11;       // imm[11]
    imm |= ((instr >> 12) & 0xFF) << 12;      // imm[19:12]
    // sign-extend from 21 to 32 bits
    if (imm & (1 << 20)) imm |= 0xFFE00000;

    // Cycle 4: A ← PC  (PC is already PC_old + 4 after fetch)
    clk++;
    A.write(PC.read());
    B.write(imm);
    print_state();

    // Cycle 5: ALUOut ← A + imm  (compute jump target)
    clk++;
    ALUOut.write(A.read() + B.read());
    print_state();

    // Cycle 6: RegFile[rd] ← A  (write return address = PC_old + 4)
    clk++;
    if (rd != 0) regfile[rd].write(A.read());
    print_state();

    // Cycle 7: PC ← ALUOut  (perform the jump)
    clk++;
    PC.write(ALUOut.read());
    print_state();
    reset_clk();
}

void Simulator::writeWord(uint32_t input, uint32_t address) {
        //if (address % 4 != 0) {
        //    cerr << "Error: Unaligned word write to address 0x" << hex << address << endl;
        //    return;
        //}
        mem[address / 4] = input;
}

void Simulator::writeHalf(uint16_t input, uint32_t address) {
    uint32_t wordAddress = address / 4;
    uint32_t offset = address % 4;
    if (offset == 0) {  //low half 
        uint32_t word = mem[wordAddress];
        word &= 0xFFFF0000;
        word |= input;
        mem[wordAddress] = word;
    }
    else if (offset == 2) {      //high half
        uint32_t word = mem[wordAddress];
        word &= 0x0000FFFF;
        word |= (input << 16);
        mem[wordAddress] = word;
    }
    else {
        cerr << "Error: .half must be aligned to 2 bytes (offset 0 or 2)" << endl;
    }
}
void Simulator::writeByte(uint8_t input, uint32_t address) {
    uint32_t wordAddress = address / 4;
    uint32_t byteOffset = address % 4;
    uint32_t oldWord = mem[wordAddress];
    oldWord &= ~(0xFF << (byteOffset * 8));        // clear that byte
    oldWord |= (input << (byteOffset * 8));        // set byte to new value
    mem[wordAddress] = oldWord;
}

