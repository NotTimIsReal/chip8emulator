#pragma once
typedef unsigned char BYTE;
typedef unsigned short int WORD;
#include <vector>
#include <string>
#include <SDL2/SDL.h>
class Emulator
{
public:
    Emulator(SDL_Renderer *window);
    void loadGame(std::string fileName);
    void render();
    void setKey(int key);
    void updateTimers();
    void run();

private:
    BYTE m_GameMemory[4096];   // 0xFFF bytes of memory
    BYTE m_Registers[16];      // 16 registers, 1 byte each
    WORD m_AddressI;           // the 16-bit address register I
    WORD m_ProgramCounter;     // the 16-bit program counter
    std::vector<WORD> m_Stack; // the 16-bit stack
    std::string m_filename;
    BYTE m_ScreenData[640][320];
    int m_pixelHeight[640][320];
    SDL_Renderer *m_window;
    int m_CurrentKeyDown;
    BYTE m_currentOpcode;
    BYTE m_Keypad[16];
    BYTE m_delayTimer;
    BYTE m_soundTimer;
    void OpcodeANNN(WORD opcode);
    void Opcode6XNN(WORD opcode);
    void Opcode00E0(WORD opcode);
    void Opcode00EE(WORD opcode);
    void OpcodeCXNN(WORD opcode);
    void Opcode9XY0(WORD opcode);
    void CPUReset();
    void Opcode8XY2(WORD opcode);
    void Opcode4XNN(WORD opcode);
    void Opcode8XY4(WORD opcode);
    void OpcodeFX90(WORD opcode);
    void OpcodeFX0A(WORD opcode);
    void Opcode7XNN(WORD opcode);
    void Opcode3XNN(WORD opcode);
    void Opcode2NNN(WORD opcode);
    void Opcode5XY0(WORD opcode);
    void Opcode8XY5(WORD opcode);
    void Opcode1NNN(WORD opcode);
    void OpcodeDXYN(WORD opcode);
    void OpcodeFX33(WORD opcode);
    WORD GetNextOpcode();
    void OpcodeFX55(WORD opcode);
    void OpcodeEX9E(WORD opcode);
    void OpcodeEXA1(WORD opcode);
    void OpcodeFX18(WORD opcode);
};