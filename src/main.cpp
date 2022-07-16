#include <SDL2/SDL.h>
#include "emulator.h"
// Update rate in hz
constexpr float timerUpdateRate = 60;
constexpr float cpuUpdateRate = 600;

// How many ticks before timers have to be updated
constexpr float cpuTicksToDo = cpuUpdateRate / timerUpdateRate;
float currentCpuTicks = 0;

// How many ms per tick
constexpr float tickFrequency = (1.0 / cpuUpdateRate) * 1000.0;

std::chrono::time_point<std::chrono::steady_clock> lastTime{};
std::chrono::duration<float, std::milli> accumulator{};
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    };
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("CHIP8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    bool isOpen = true;
    SDL_Event event;
    Emulator emulator(renderer);
    emulator.loadGame(argv[1]);

    while (isOpen)
    {
        accumulator += (std::chrono::high_resolution_clock::now() - lastTime);

        if (accumulator.count() >= tickFrequency)
        {

            currentCpuTicks++;
            accumulator = std::chrono::milliseconds::zero();
        }
        else
        {

            lastTime = std::chrono::high_resolution_clock::now();
            continue;
        }
        if (currentCpuTicks < cpuTicksToDo)
        {
            emulator.run();
            emulator.render();
            while (SDL_PollEvent(&event))
            {

                if (event.type == SDL_QUIT)
                {
                    isOpen = false;
                    break;
                }
                else if (event.type == SDL_KEYDOWN)
                {
                    emulator.setKey(event.key.keysym.sym);
                }
            }
            // else
            // {
            //     continue;
            // }
        }
        else
        {
            currentCpuTicks = 0;
            emulator.updateTimers();
        }
    };
}