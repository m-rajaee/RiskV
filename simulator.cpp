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

void Simulator::choose_clk_type()
{
    std::string input;
    while (true)
    {
        cout << "\033[1;91mChoose clk type: A(Auto), M(Manual): \033[0m";
        getline(cin, input);

        if (input.size() == 1)
        {
            clk_type = toupper(static_cast<unsigned char>(input[0]));
            if (clk_type == 'A' || clk_type == 'M')
                break;
        }
        cout << "\033[1;91mInvalid input. Please enter 'A' or 'M'.\033[0m";
    }

    if (clk_type == 'A')
    {
        cout << "\033[1;91mChoose the speed (Hz) (0 for max): \033[0m";
        cin >> clk_speed;
        if (clk_speed != 0)
            delay = 1'000'000.0 / clk_speed;
    }
}

void Simulator::pause()
{
    if (clk_speed == 0)
    {
        return;
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(delay)));
    }
}

void Simulator::wait_for_user()
{
    char c;
    cout << "Press Enter to continue...\n";
    _getch();
}

void Simulator::print_state()
{
    cout << "\033[1;36m================ CLOCK CYCLE: " << clk << " =================\033[0m\n";

    for (int i = 0; i < 32; i += 4)
    {
        cout << "\033[1;33m" << setw(3) << reg_names[i] << ":\033[0m "
            << "\033[0;32m" << setw(8) << setfill('0') << hex << regfile[i].read() << "\033[0m  "
            << "\033[1;33m" << setw(3) << reg_names[i + 1] << ":\033[0m "
            << "\033[0;32m" << setw(8) << regfile[i + 1].read() << "\033[0m  "
            << "\033[1;33m" << setw(3) << reg_names[i + 2] << ":\033[0m "
            << "\033[0;32m" << setw(8) << regfile[i + 2].read() << "\033[0m  "
            << "\033[1;33m" << setw(3) << reg_names[i + 3] << ":\033[0m "
            << "\033[0;32m" << setw(8) << regfile[i + 3].read() << "\033[0m\n";
    }

    cout << "\n\033[1;36m[Processor State Registers]\033[0m\n";
    cout << "\033[1;35m PC     :\033[0m \033[0;36m" << setw(8) << PC.read()
        << "\033[0m  \033[1;35mMAR    :\033[0m \033[0;36m" << setw(8) << MAR.read()
        << "\033[0m  \033[1;35mMDR    :\033[0m \033[0;36m" << setw(8) << MDR.read() << "\n";
    cout << "\033[1;35m IR     :\033[0m \033[0;36m" << setw(8) << IR.read()
        << "\033[0m  \033[1;35mA      :\033[0m \033[0;36m" << setw(8) << A.read()
        << "\033[0m  \033[1;35mB      :\033[0m \033[0;36m" << setw(8) << B.read() << "\n";
    cout << "\033[1;35m ALUOut :\033[0m \033[0;36m" << setw(8) << ALUOut.read() << "\033[0m\n";

    cout << "\033[1;36m================================================\033[0m\n\n";


    if (clk_type == 'A')
        pause();
    else
        wait_for_user();

}

void Simulator::start()
{
    choose_clk_type();
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
    int rd     = (instr >> 7)  & 0x1F;
    int funct3 = (instr >> 12) & 0x7;
    int rs1    = (instr >> 15) & 0x1F;
    int rs2    = (instr >> 20) & 0x1F;
    int funct7 = (instr >> 25) & 0x7F;

    // Cycle 4: Read registers into A and B
    clk++;
    A.write(regfile[rs1].read());
    B.write(regfile[rs2].read());
    print_state();

    // Cycle 5: Compute ALU result
    clk++;
    int32_t   opA = A.read();
    int32_t   opB = B.read();
    int32_t   val = 0;

    switch (funct3)
    {
      // add, sub, mul
      case 0x0:
        if      (funct7 == 0x00) val = opA + opB;
        else if (funct7 == 0x20) val = opA - opB;
        else if (funct7 == 0x01) // mul
        {
          int64_t prod = int64_t(opA) * int64_t(opB);
          val = int32_t(prod);  // low 32 bits
        }
        break;

      // sll, mulh (signed×signed → upper 32 bits)
      case 0x1:
        if      (funct7 == 0x00)
          val = opA << (opB & 0x1F);
        else if (funct7 == 0x01)
        {
          int64_t prod = int64_t(opA) * int64_t(opB);
          val = int32_t(prod >> 32);
        }
        break;

      // slt, mulhsu (signed×unsigned → upper 32 bits)
      case 0x2:
        if      (funct7 == 0x00)
          val = (opA < opB);
        else if (funct7 == 0x01)
        {
          // sign-extend opA, zero-extend opB, full 64-bit product
          uint64_t a64 = uint64_t(int64_t(opA));
          uint64_t b64 = uint64_t(uint32_t(opB));
          uint64_t prod = a64 * b64;
          val = int32_t(prod >> 32);
        }
        break;

      // sltu, mulhu (unsigned×unsigned → upper 32 bits)
      case 0x3:
        if      (funct7 == 0x00)
          val = (uint32_t(opA) < uint32_t(opB));
        else if (funct7 == 0x01)
        {
          uint64_t prod = uint64_t(uint32_t(opA)) * uint64_t(uint32_t(opB));
          val = int32_t(prod >> 32);
        }
        break;

      // xor, div
      case 0x4:
        if      (funct7 == 0x00)
          val = opA ^ opB;
        else if (funct7 == 0x01)
        {
          if      (opB == 0)                       val = -1;
          else if (opA == INT32_MIN && opB == -1)  val = opA;  // overflow case
          else                                     val = opA / opB;
        }
        break;

      // srl, sra, divu
      case 0x5:
        if      (funct7 == 0x00)
          val = int32_t(uint32_t(opA) >> (opB & 0x1F));
        else if (funct7 == 0x20)
          val = opA >> (opB & 0x1F);
        else if (funct7 == 0x01)
        {
          uint32_t uA = uint32_t(opA);
          uint32_t uB = uint32_t(opB);
          val = int32_t(uB == 0 ? UINT32_MAX : uA / uB);
        }
        break;

      // or, rem
      case 0x6:
        if      (funct7 == 0x00)
          val = opA | opB;
        else if (funct7 == 0x01)
        {
          if      (opB == 0)                      val = opA;
          else if (opA == INT32_MIN && opB == -1) val = 0;
          else                                    val = opA % opB;
        }
        break;

      // and, remu
      case 0x7:
        if      (funct7 == 0x00)
          val = opA & opB;
        else if (funct7 == 0x01)
        {
          uint32_t uA = uint32_t(opA);
          uint32_t uB = uint32_t(opB);
          val = int32_t(uB == 0 ? uA : (uA % uB));
        }
        break;
    }

    ALUOut.write(val);
    print_state();

    // Cycle 6: Write-back
    clk++;
    if (rd != 0)
        regfile[rd].write(ALUOut.read());
    print_state();

    reset_clk();
}

void Simulator::I_type(uint32_t instr, uint32_t opcode)
{
    int rd     = (instr >> 7)  & 0x1F;
    int funct3 = (instr >> 12) & 0x7;
    int rs1    = (instr >> 15) & 0x1F;
    // sign-extend 12-bit imm
    int32_t imm = int32_t(instr) >> 20;

    switch (opcode)
    {
      // --------------------------------------------------
      // I-type arithmetic & shifts (0x13)
      // funct3: 0=addi,1=slli,4=xori,6=ori,7=andi,5=srli/srai…
      // --------------------------------------------------
      case 0x13:
      {
        // 1) fetch A and immediate
        clk++;
        A.write(regfile[rs1].read());
        B.write(imm);
        print_state();

        // 2) ALU operation depends on funct3
        clk++;
        {
          int32_t a = A.read();
          int32_t b = B.read();
          uint32_t ua = uint32_t(a);
          int32_t result;

          switch (funct3)
          {
            case 0x0: // addi
              result = a + b;
              break;

            case 0x1: // slli (only low 5 bits of imm)
            {
              int sh = b & 0x1F;
              result = a << sh;
              break;
            }

            case 0x5: // srli (imm[10]==0) – logical
            {
              int sh = b & 0x1F;
              result = int32_t(ua >> sh);
              break;
            }

            case 0x6: // ori
              result = a | b;
              break;

            case 0x7: // andi
              result = a & b;
              break;

            default:
              result = 0;
              break;
          }

          ALUOut.write(result);
        }
        print_state();

        // 3) write-back
        clk++;
        if (rd != 0)
          regfile[rd].write(ALUOut.read());
        print_state();
        break;
      }

      // --------------------------------------------------
      // Loads: lb, lh, lw, lbu, lhu  (0x03)
      // funct3: 0=lb,1=lh,2=lw,4=lbu,5=lhu
      // --------------------------------------------------
      case 0x03:
      {
        // 1) compute address
        clk++;
        A.write(regfile[rs1].read());
        B.write(imm);
        print_state();

        clk++;
        ALUOut.write(A.read() + B.read());
        print_state();

        // 2) memory fetch
        clk++;
        MAR.write(ALUOut.read());
        print_state();

        clk++;
        {
          uint32_t addr = MAR.read();
          // word-aligned index
          uint32_t idxW = (addr & ~0x3) / 4;
          uint32_t dataW = (idxW < MEM_SIZE) ? mem[idxW] : 0;
          // half-word index
          uint32_t idxH = (addr & ~0x1) / 4;
          uint32_t dataH = (idxH < MEM_SIZE) ? mem[idxH] : 0;
          int     offH  = (addr & 0x2) ? 16 : 0;
          int16_t valH  = (dataH >> offH) & 0xFFFF;
          int8_t  valB  = (dataW >> ((addr & 0x3) * 8)) & 0xFF;

          int32_t loaded;
          switch (funct3)
          {
            case 0x0: // lb
              loaded = int32_t(valB);      // sign-extend byte
              break;
            case 0x1: // lh
              loaded = int32_t(valH);      // sign-extend half
              break;
            case 0x2: // lw
              loaded = int32_t(dataW);
              break;
            case 0x4: // lbu
              loaded = uint32_t(valB) & 0xFF;    // zero-extend byte
              break;
            case 0x5: // lhu
              loaded = uint32_t(valH) & 0xFFFF;  // zero-extend half
              break;
            default:
              loaded = 0;
          }
          MDR.write(loaded);
        }
        print_state();

        // 3) write-back
        clk++;
        if (rd != 0)
          regfile[rd].write(MDR.read());
        print_state();
        break;
      }

      case 0x67:
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

    // Cycle 8: Write data from MDR into memory (sb, sh, sw)
    clk++;
    {
        uint32_t addr = MAR.read();
        uint32_t addrWord = (addr & ~0x3u) / 4;
        if (addrWord < MEM_SIZE)
        {
            uint32_t curr = mem[addrWord];
            switch (funct3)
            {
            case 0x0:  // sb: store byte
            {
                uint8_t byte = uint8_t(MDR.read() & 0xFF);
                int shift = (addr & 0x3) * 8;
                curr = (curr & ~(0xFFu << shift)) | (uint32_t(byte) << shift);
                mem[addrWord] = curr;
                break;
            }
            case 0x1:  // sh: store halfword
            {
                uint16_t half = uint16_t(MDR.read() & 0xFFFF);
                if (addr & 0x2)
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


    // Cycle 5: If branch condition met, update PC with branch target.
    if (takeBranch)
    {
        PC.write(branchTarget);
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
    if (rd != 0)
        regfile[rd].write(PC.read());
    print_state();

    // Cycle 5: ALUOut ← A + imm  (compute jump target)
    clk++;
    PC.write(PC.read() + imm);
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