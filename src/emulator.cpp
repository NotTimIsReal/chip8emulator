#include "emulator.h"
#include <vector>
// declare fontset
const BYTE fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
// keypad to sdl2 keycode
const int keypad[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3, SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c, SDLK_4, SDLK_r, SDLK_f, SDLK_v};
void Emulator::CPUReset()
{
    m_AddressI = 0;
    m_ProgramCounter = 0x200;
    memset(m_Registers, 0, sizeof(m_Registers)); // set registers to 0

    for (int i = 0; i < 80; i++)
    {
        m_GameMemory[i] = fontset[i];
    }
    FILE *in;
    in = fopen(m_filename.c_str(), "rb");
    fread(&m_GameMemory[0x200], 0x1000, 1, in);
    fclose(in);
}

void Emulator::loadGame(std::string fileName)
{
    m_filename = fileName;
    CPUReset();
}
Emulator::Emulator(SDL_Renderer *window)
{
    m_window = window;
}
void Emulator::render()
{
    SDL_SetRenderDrawColor(m_window, 0, 0, 0, 255);
    SDL_RenderClear(m_window);
    SDL_SetRenderDrawColor(m_window, 255, 255, 255, 255);
    for (int y = 0; y < 320; y++)
    {
        for (int x = 0; x < 640; x++)
        {
            if (m_ScreenData[x][y] == 1)
            {

                // draw a pixel

                SDL_Rect pixel = {
                    x,
                    y,
                    8,
                    m_pixelHeight[x][y]};
                SDL_RenderFillRect(m_window, &pixel);
            }
        }
    }
    SDL_RenderPresent(m_window);
}

WORD Emulator::GetNextOpcode()
{

    WORD res = 0;
    res = m_GameMemory[m_ProgramCounter]; // in example res is 0xAB
    res <<= 8;                            // shift 8 bits left. In our example res is 0xAB00
    res |= m_GameMemory[m_ProgramCounter + 1];
    // In example res is 0xABCD m_ProgramCounter += 2;
    m_ProgramCounter += 2;
    return res;
}
void Emulator::Opcode1NNN(WORD opcode)
{
    m_ProgramCounter = opcode & 0x0FFF; // remember we're only interested in NNN of opcode 1NNN
}
void Emulator::setKey(int key)
{
    m_CurrentKeyDown = key;
    for (int i = 0; i < 16; i++)
    {
        if (key == keypad[i])
        {
            m_Keypad[i] = 1;
            break;
        }
    }
}
void Emulator::Opcode2NNN(WORD opcode)
{
    m_Stack.push_back(m_ProgramCounter); // save the program counter
    m_ProgramCounter = opcode & 0x0FFF;  // jump to address NNN
}
void Emulator::Opcode5XY0(WORD opcode)
{
    int regx = opcode & 0x0F00; // mask off reg x
    regx = regx >> 8;           // shift x across
    int regy = opcode & 0x00F0; // mask off reg y
    regy = regy >> 4;           // shift y across
    if (m_Registers[regx] == m_Registers[regy])
        m_ProgramCounter += 2; // skip next instruction
}
void Emulator::Opcode8XY5(WORD opcode)
{
    m_Registers[0xF] = 1;
    int regx = opcode & 0x0F00; // mask off reg x
    regx = regx >> 8;           // shift x across
    int regy = opcode & 0x00F0; // mask off reg y
    regy = regy >> 4;           // shift y across
    int xval = m_Registers[regx];
    int yval = m_Registers[regy];
    if (yval > xval) // if this is true will result in a value < 0
        m_Registers[0xF] = 0;
    m_Registers[regx] = xval - yval;
}
void Emulator::OpcodeDXYN(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int regy = opcode & 0x00F0;
    regy = regy >> 4;
    int height = opcode & 0x000F;
    int coordx = m_Registers[regx];
    int coordy = m_Registers[regy];
    m_Registers[0xf] = 0;

    // loop for the amount of vertical lines needed to draw
    for (int yline = 0; yline < height; yline++)
    {
        BYTE data = m_GameMemory[m_AddressI + yline];
        int xpixelinv = 7;
        int xpixel = 0;
        for (xpixel = 0; xpixel < 8; xpixel++, xpixelinv--)
        {
            int mask = 1 << xpixelinv;
            if (data & mask)
            {
                int x = (coordx + xpixel) * 10;
                int y = (coordy + yline) * 10;
                if (m_ScreenData[x][y] == 1)
                    m_Registers[0xF] = 1; // collision
                m_ScreenData[x][y] ^= 1;
                m_pixelHeight[x][y] = height;
            }
        }
    }
}
void Emulator::OpcodeFX33(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx >>= 8;

    int value = m_Registers[regx];

    int hundreds = value / 100;
    int tens = (value / 10) % 10;
    int units = value % 10;

    m_GameMemory[m_AddressI] = hundreds;
    m_GameMemory[m_AddressI + 1] = tens;
    m_GameMemory[m_AddressI + 2] = units;
}
void Emulator::OpcodeFX55(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx >>= 8;
    for (int i = 0; i <= regx; i++)
    {
        m_GameMemory[m_AddressI + i] = m_Registers[i];
    }
    m_AddressI = m_AddressI + regx + 1;
}
void Emulator::run()
{
    WORD opcode = GetNextOpcode();
    printf("Opcode: %X\n", opcode);
    switch (opcode & 0xF000)
    {
    case 0x1000:
        Opcode1NNN(opcode);
        break; // jump opcode
    case 0x0000:
        switch (opcode & 0x000F)
        {
        case 0x00E0:
            Opcode00E0(opcode);
            break; // clear screen opcode
        case 0x00EE:
            Opcode00EE(opcode);
            break; // return from subroutine opcode
        }
    case 0x2000:
        Opcode2NNN(opcode);
        break; // call subroutine opcode
    case 0x5000:
        Opcode5XY0(opcode);
        break; // skip if equal opcode
    case 0x8000:
        switch (opcode & 0x000F)
        {
        case 0x0005:
            Opcode8XY5(opcode);
            break; // bitwise or opcode
        case 0x0002:
            Opcode8XY2(opcode);
            break; // bitwise and opcode
        case 0x0004:
            Opcode8XY4(opcode);
            break; // bitwise xor opcode
        default:
            printf("Unknown opcode: %X\n", opcode);
            break;
        }
        break; // bitwise or opcode
    case 0xD000:
        OpcodeDXYN(opcode);
        break; // draw sprite opcode
    case 0x4000:
        Opcode4XNN(opcode);
        break; // skip if not equal opcode
    case 0xF000:
        switch (opcode & 0x00FF)
        {
        case 0x033:
            OpcodeFX33(opcode);
            break;
        case 0x055:
            OpcodeFX55(opcode);
            break;
        case 0x090:
            OpcodeFX90(opcode);
            break;
        case 0x00A:
            OpcodeFX0A(opcode);
            break;
        case 0x0015:
            OpcodeFX15(opcode);
            break;
        case 0x0018:
            OpcodeFX18(opcode);
            break;
        default:
            printf("Unknown opcode: %X\n", opcode);
            break;
        }
    case 0xA000:
        OpcodeANNN(opcode);
        break; // load address into I opcode
    case 0x6000:
        Opcode6XNN(opcode);
        break; // load constant into register opcode

    case 0x9000:
        Opcode9XY0(opcode);
        break; // skip if not equal opcode
    case 0x7000:
        Opcode7XNN(opcode);
        break; // load constant into register opcode
    case 0x3000:
        Opcode3XNN(opcode);
        break; // load constant into register opcode
    case 0xE000:
        switch (opcode & 0x00FF)
        {
        case 0x009E:
            OpcodeEX9E(opcode);
            break; // skip if key pressed opcode
        case 0x00A1:
            OpcodeEXA1(opcode);
            break; // skip if key not pressed opcode
        }
        break;
    case 0xC000:
        OpcodeCXNN(opcode);
        break; // skip if equal opcode
    default:
        printf("Unknown opcode: %X\n", opcode);
        break; // opcode yet to be handled
    }
}
void Emulator::OpcodeANNN(WORD opcode)
{
    m_AddressI = opcode & 0x0FFF;
}
void Emulator::Opcode6XNN(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int value = opcode & 0x00FF;
    m_Registers[regx] = value;
}
void Emulator::Opcode00E0(WORD opcode)
{
    for (int i = 0; i < 640; i++)
    {
        for (int j = 0; j < 320; j++)
        {
            m_ScreenData[i][j] = 0;
        }
    }
}
void Emulator::Opcode00EE(WORD opcode)
{
    m_ProgramCounter = m_Stack.back();
    m_Stack.pop_back();
}

void Emulator::Opcode9XY0(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int regy = opcode & 0x00F0;
    regy = regy >> 4;
    if (m_Registers[regx] != m_Registers[regy])
        m_ProgramCounter += 2;
}
void Emulator::OpcodeFX90(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int regy = opcode & 0x00F0;
    regy = regy >> 4;
    if (m_Registers[regx] == m_Registers[regy])
        m_ProgramCounter += 2;
}
void Emulator::Opcode7XNN(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int value = opcode & 0x00FF;
    // add value to register
    m_Registers[regx] += value;
}
void Emulator::Opcode3XNN(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int value = opcode & 0x00FF;
    // Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block);
    if (m_Registers[regx] == value)
        m_ProgramCounter += 2;
}
void Emulator::OpcodeEX9E(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    if (m_Keypad[m_Registers[regx]] == 1)

        m_ProgramCounter += 2;
}
void Emulator::OpcodeEXA1(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    if (m_Keypad[m_Registers[regx]] != 0)
        m_ProgramCounter += 2;
}
void Emulator::OpcodeCXNN(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int value = opcode & 0x00FF;
    m_Registers[regx] = (rand() % 256) & value;
}
void Emulator::Opcode8XY2(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int regy = opcode & 0x00F0;
    regy = regy >> 4;
    m_Registers[regx] = m_Registers[regx] & m_Registers[regy];
}
void Emulator::Opcode4XNN(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int value = opcode & 0x00FF;
    if (m_Registers[regx] != value)
        m_ProgramCounter += 2;
}
void Emulator::Opcode8XY4(WORD opcode)
{
    // Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int regy = opcode & 0x00F0;
    regy = regy >> 4;
    if (m_Registers[regx] + m_Registers[regy] > 255)
    {
        m_Registers[0xF] = 1;
        m_Registers[regx] = m_Registers[regx] + m_Registers[regy] - 256;
    }
    else
    {
        m_Registers[0xF] = 0;
        m_Registers[regx] += m_Registers[regy];
    }
}
void Emulator::OpcodeFX0A(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    int key = 0;
    for (int i = 0; i < 16; i++)
    {
        if (m_Keypad[i] == 1)
        {
            key = i;
            break;
        }
    }
    m_Registers[regx] = key;
}
void Emulator::updateTimers()
{
    if (m_delayTimer > 0)
        m_delayTimer--;
    if (m_delayTimer > 0)
    {
        m_soundTimer--;
        if (m_soundTimer == 0)
        {
            printf("BEEP\n");
            m_soundTimer = 60;
        }
    }
}

void Emulator::FX18(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    m_SoundTimer = m_Registers[regx];
}
void Emulator::FX15(WORD opcode)
{
    int regx = opcode & 0x0F00;
    regx = regx >> 8;
    m_DelayTimer = m_Registers[regx];
}