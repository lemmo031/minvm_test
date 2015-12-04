//
// MinVM Take Home Test
//
// Implement a minimal VM  
//
// See README.md for full instructions
//
// Author: Aaron Lemmon, a.lemmon777@gmail.com
//

#include "minvm_defs.h"

static const byte registerMasks[] = { REGA, REGB, REGC, REGD };
static const byte bitCountLookup[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 }; // Used to look up the number of one bits in a half-word
void loadi (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void inc (byte *registers[], byte operandRegisterMask);
void dec (byte *registers[], byte operandRegisterMask);
void loadr (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void add (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void sub (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void mul (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void div (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void and (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void or (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void xor (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask);
void rotr (byte *registers[], byte operandRegisterMask);
void jmpneq (virtual_machine_t *vm, byte *registers[], byte operandRegisterMask);
void jmpeq (virtual_machine_t *vm, byte *registers[], byte operandRegisterMask);
void stor (virtual_machine_t *vm, byte *registers[], byte sourceRegisterMask);
void itr (virtual_machine_t *vm, byte interruptFunctionIndex);
unsigned long getLongFromRegisters(byte *registers[], byte operandRegisterMask);
bool isValidSourceRegisterMask (byte sourceRegisterMask, byte numRequiredRegisters);
byte getRelevantRegisters (byte *relevantRegisters[], byte *allRegisters[], byte registerMask);
void getOperands (byte operands[], byte *registers[], byte sourceRegisterMask);
void storeLongResultInRegisters (unsigned long result, byte *registers[], byte destinationRegisterMask);
void storeByteInEachRegister (byte result, byte *registers[], byte destinationRegisterMask);

// Implement your VM here
void vm_exec (virtual_machine_t *vm) {
    byte *registers[NUM_REGISTERS]; // Used to loop over the registers, making the code less verbose
    registers[0] = &(vm->a); // Unfortunately, cl.exe won't let me inline these
    registers[1] = &(vm->b);
    registers[2] = &(vm->c);
    registers[3] = &(vm->d);
    while (!(vm->flags & MINVM_HALT)) { // Continue running if the halt flag is not set
        byte instruction = vm->code[vm->pc++]; // Increments the program counter past the instruction
        byte opcode = 0xF0 & instruction; // The upper 4 bits of the instruction
        byte argument = 0x0F & instruction; // The lower 4 bits of the instruction
        switch (opcode) {
            case 0x00: // LOADI
                loadi(vm, registers, argument); break;
            case 0x10: // INC
                inc(registers, argument); break;
            case 0x20: // DEC
                dec(registers, argument); break;
            case 0x30: // LOADR
                loadr(vm, registers, argument); break;
            case 0x40: // ADD
                add(vm, registers, argument); break;
            case 0x50: // SUB
                sub(vm, registers, argument); break;
            case 0x60: // MUL
                mul(vm, registers, argument); break;
            case 0x70: // DIV
                div(vm, registers, argument); break;
            case 0x80: // AND
                and(vm, registers, argument); break;
            case 0x90: // OR
                or(vm, registers, argument); break;
            case 0xA0: // XOR
                xor(vm, registers, argument); break;
            case 0xB0: // ROTR
                rotr(registers, argument); break;
            case 0xC0: // JMPNEQ
                jmpneq(vm, registers, argument); break;
            case 0xD0: // JMPEQ
                jmpeq(vm, registers, argument); break;
            case 0xE0: // STOR
                stor(vm, registers, argument); break;
            case 0xF0: // ITR
                itr(vm, argument); break;
        }
    }
}

void loadi (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte *destinationRegisters[NUM_REGISTERS];
    byte count;
    byte index;
    if (destinationRegisterMask == 0x00) { // This code halts the virtual machine
        vm->flags = MINVM_HALT;
        return;
    }
    count = getRelevantRegisters(destinationRegisters, registers, destinationRegisterMask);
    for (index = 0; index < count; ++index) {
        *destinationRegisters[index] = vm->code[vm->pc++]; // Write to the destination registers
    }
}

void inc (byte *registers[], byte operandRegisterMask) {
    unsigned long temp = getLongFromRegisters(registers, operandRegisterMask);
    ++temp;
    storeLongResultInRegisters(temp, registers, operandRegisterMask);
}

void dec (byte *registers[], byte operandRegisterMask) {
    unsigned long temp = getLongFromRegisters(registers, operandRegisterMask);
    --temp;
    storeLongResultInRegisters(temp, registers, operandRegisterMask);
}

void loadr (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte countOfDestinationRegisters = bitCountLookup[destinationRegisterMask];
    byte data[NUM_REGISTERS]; // Bytes read from code locations referenced from the source registers will be stored here temporarily
    byte *sourceRegisters[NUM_REGISTERS];
    byte *destinationRegisters[NUM_REGISTERS];
    byte count;
    byte index;

    if (!isValidSourceRegisterMask(sourceRegisterMask, countOfDestinationRegisters)) { // The count of source registers must equal the count of destination registers
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    count = getRelevantRegisters(sourceRegisters, registers, sourceRegisterMask);
    for (index = 0; index < count; ++index) {
        data[index] = vm->code[*sourceRegisters[index]]; // Copy values from code to data array
    }

    count = getRelevantRegisters(destinationRegisters, registers, destinationRegisterMask);
    for (index = 0; index < count; ++index) {
        *destinationRegisters[index] = data[index]; // Copy values to destination registers
    }
}

void add (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    unsigned long result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers
    result = (unsigned long)*operands[0] + (unsigned long)*operands[1];
    storeLongResultInRegisters(result, registers, destinationRegisterMask); // Store the result back to the destination registers
}

void sub (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    unsigned long result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers
    result = (unsigned long)*operands[0] - (unsigned long)*operands[1];
    storeLongResultInRegisters(result, registers, destinationRegisterMask); // Store the result back to the destination registers
}

void mul (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    unsigned long result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers
    result = (unsigned long)*operands[0] * (unsigned long)*operands[1];
    storeLongResultInRegisters(result, registers, destinationRegisterMask); // Store the result back to the destination registers
}

void div (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    unsigned long result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers

    if (*operands[1] == 0x00) { // Cannot divide by zero
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    result = (unsigned long)*operands[0] / (unsigned long)*operands[1];
    storeLongResultInRegisters(result, registers, destinationRegisterMask); // Store the result back to the destination registers
}

void and (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    byte result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers
    result = *operands[0] & *operands[1];
    storeByteInEachRegister(result, registers, destinationRegisterMask);
}

void or (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    byte result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers
    result = *operands[0] | *operands[1];
    storeByteInEachRegister(result, registers, destinationRegisterMask);
}

void xor (virtual_machine_t *vm, byte *registers[], byte destinationRegisterMask) {
    byte sourceRegisterMask = vm->code[vm->pc++];
    byte *operands[NUM_REGISTERS];
    byte result;

    if (!isValidSourceRegisterMask(sourceRegisterMask, 2)) { // The count of source registers must equal 2
        vm->flags = MINVM_EXCEPTION | MINVM_HALT;
        return;
    }

    getRelevantRegisters(operands, registers, sourceRegisterMask); // Gets the two source registers
    result = *operands[0] ^ *operands[1];
    storeByteInEachRegister(result, registers, destinationRegisterMask);
}

void rotr (byte *registers[], byte operandRegisterMask) {
    byte *rotatingRegisters[NUM_REGISTERS];
    byte count = getRelevantRegisters(rotatingRegisters, registers, operandRegisterMask);
    byte index;
    if (count >= 2) {// A count of less than two should result in a no-op
        byte temp = *rotatingRegisters[count - 1];
        for (index = count - 1; index > 0; --index) {
            *rotatingRegisters[index] = *rotatingRegisters[index - 1];
        }
        *rotatingRegisters[0] = temp;
    }
}

void jmpneq (virtual_machine_t *vm, byte *registers[], byte operandRegisterMask) {
    byte jumpLocation = vm->code[vm->pc++];
    byte countOfOperandRegisters = bitCountLookup[operandRegisterMask];
    byte operands[NUM_REGISTERS]; // Storage for relevant registers
    int index = 0;
    int i;
    if (operandRegisterMask == 0x00) { // This code signals an unconditional jump
        vm->pc = jumpLocation;
        return;
    }
    if (countOfOperandRegisters == 1) { // Jump if the value of the single register is not 0
        switch (operandRegisterMask) {
            case REGA:
                index = 0; break;
            case REGB:
                index = 1; break;
            case REGC:
                index = 2; break;
            case REGD:
                index = 3; break;
        }
        if (*registers[index] != 0x00) {
            vm->pc = jumpLocation;
        }
        return;
    }
    getOperands(operands, registers, operandRegisterMask); // Copy values of relevant registers to operands array
    for (i = 1; i < countOfOperandRegisters; ++i) {
        if (operands[0] != operands[i]) { // Found different values
            vm->pc = jumpLocation;
        }
    }
    // If we get here, no jump
}

void jmpeq (virtual_machine_t *vm, byte *registers[], byte operandRegisterMask) {
    byte jumpLocation = vm->code[vm->pc++];
    byte countOfOperandRegisters = bitCountLookup[operandRegisterMask];
    byte operands[NUM_REGISTERS]; // Storage for relevant registers
    int index = 0;
    int i;
    if (operandRegisterMask == 0x00) { // This code signals an unconditional jump
        vm->pc = jumpLocation;
        return;
    }
    if (countOfOperandRegisters == 1) { // Jump if the value of the single register is not 0
        switch (operandRegisterMask) {
        case REGA:
            index = 0; break;
        case REGB:
            index = 1; break;
        case REGC:
            index = 2; break;
        case REGD:
            index = 3; break;
        }
        if (*registers[index] == 0x00) {
            vm->pc = jumpLocation;
        }
        return;
    }
    getOperands(operands, registers, operandRegisterMask); // Copy values of relevant registers to operands array
    for (i = 1; i < countOfOperandRegisters; ++i) {
        if (operands[0] != operands[i]) { // Found different values

            return;
        }
    }
    // If we get here, all relevant registers were equal, so jump
    vm->pc = jumpLocation;
}

void stor (virtual_machine_t *vm, byte *registers[], byte sourceRegisterMask) {
    byte storeLocation = vm->code[vm->pc++];
    byte countOfOperandRegisters = bitCountLookup[sourceRegisterMask];
    byte valuesToStore[NUM_REGISTERS]; // Relevant register values are temporarily stored here
    int i;

    getOperands(valuesToStore, registers, sourceRegisterMask); // Copy values of relevant registers to valuesToStore
    
    for (i = 0; i < countOfOperandRegisters; ++i) {
        vm->code[storeLocation] = valuesToStore[i];
        ++storeLocation;
    }
}

void itr (virtual_machine_t *vm, byte interruptFunctionIndex) {
    (vm->interrupts[interruptFunctionIndex])(vm); // Calls the interrupt function specified by the index
}

// Concatenates values in the specified registers into a single unsigned long
unsigned long getLongFromRegisters(byte *registers[], byte operandRegisterMask) {
    int i;
    unsigned long temp = 0; // Relevent registers will be copied here, using a 32 bit unsigned integer to handle overflow
    for (i = NUM_REGISTERS - 1; i >= 0; --i) { // Start with register D, since it should be the most significant byte
        if (operandRegisterMask & registerMasks[i]) {
            temp = temp << WORD_SIZE; // Make room for the next value (has no effect if temp is still 0)
            temp = temp | *registers[i]; // Put value of the register at the end of temp
        }
    }
    return temp;
}

// Checks that a mask has a 0 in the upper byte and encodes a number of registers exactly equal to numRequiredRegisters
bool isValidSourceRegisterMask (byte sourceRegisterMask, byte numRequiredRegisters) {
    if (sourceRegisterMask & 0xF0) { // Invalid if the upper byte is not 0
        return false;
    }
    return bitCountLookup[sourceRegisterMask] == numRequiredRegisters; // Valid if the mask has exactly the required registers
}

// Populates the relevantRegisters array with pointers to the registers specified in registerMask
// Returns the count of relevant registers
// Proceeds from register A to register D
byte getRelevantRegisters (byte *relevantRegisters[], byte *allRegisters[], byte registerMask) {
    byte index;
    byte count = 0;
    for (index = 0; index < NUM_REGISTERS; ++index) {
        if (registerMask & registerMasks[index]) {
            relevantRegisters[count++] = allRegisters[index];
        }
    }
    return count;
}

// Copies the values of the registers specified in sourceRegisterMask to the operands array
void getOperands (byte operands[], byte *registers[], byte sourceRegisterMask) {
    byte countOfSourceRegisters = bitCountLookup[sourceRegisterMask];
    int index = 0;
    int registersdone = 0;
    while (registersdone < countOfSourceRegisters) { // Copy to operands array
        if (sourceRegisterMask & registerMasks[index]) {
            operands[registersdone] = *registers[index];
            ++registersdone;
        }
        ++index;
    }
}

// Breaks the long result back into 8 byte pieces and stores them in the registers specified in destinationRegisterMask
void storeLongResultInRegisters (unsigned long result, byte *registers[], byte destinationRegisterMask) {
    byte *destinationRegisters[NUM_REGISTERS];
    byte count = getRelevantRegisters(destinationRegisters, registers, destinationRegisterMask);
    byte index;
    for (index = 0; index < count; ++index) {
        *destinationRegisters[index] = (byte)result; // Stores the least significant byte into the register
        result = result >> WORD_SIZE; // Shift values in result to prepare for next iteration
    }
}

// Stores the result byte into each register specified in destinationRegisterMask
void storeByteInEachRegister (byte result, byte *registers[], byte destinationRegisterMask) {
    byte *destinationRegisters[NUM_REGISTERS];
    byte count = getRelevantRegisters(destinationRegisters, registers, destinationRegisterMask);
    byte index;
    for (index = 0; index < count; ++index) {
        *destinationRegisters[index] = result; // Stores the byte into the register
    }
}
