/**
 * @file program.cpp
 * @brief Handles the main loop
 * @author ImpendingMoon
 * @date 2023-07-23
 */

#include "program.hpp"
#include <stdexcept>
#include <filesystem>
#include <cstdint>
#include <fmt/core.h>
#include <SDL2/SDL.h>
#include "logger.hpp"
#include "window.hpp"
#include "main.hpp"
#include "emu/emusys.hpp"

bool exitRequested = false;
double frameRate = 60;

void handleEvents(void) noexcept;
void handleKeyboard(SDL_KeyboardEvent key);

EmuSys* emuSystem = nullptr;

/**
 * @brief Runs the main program loop until an exit is requested
 */
void runMainLoop(void) noexcept
{
    logMessage("Starting main loop...", LOG_INFO);

    while(!exitRequested)
    {
        uint64_t start_time = SDL_GetPerformanceCounter();

        handleEvents();

        if(emuSystem != nullptr && emuSystem->isRunning())
        {
            try
            {
                emuSystem->runFrame();
            } catch(std::exception& ex)
            {
                logMessage(ex.what(), LOG_DEBUG);
            }
        }

        windowClear();
        windowUpdate();

        uint64_t end_time = SDL_GetPerformanceCounter();
        double seconds_elapsed =
            (end_time - start_time)
            / static_cast<double>(SDL_GetPerformanceFrequency());

        SDL_Delay(
            (seconds_elapsed < (1 / frameRate))
            ?
            static_cast<uint32_t>(
            floor((1000 / frameRate) - seconds_elapsed))
            :
            0
        );
    }

    if(emuSystem != nullptr)
    {
        emuSystem->dumpSystem();
        delete emuSystem;
    }

    logMessage("Exited main loop.", LOG_INFO);
}



/**
 * @brief Requests to exit the program at the end of the current loop
 */
void requestExit(void) noexcept
{
    logMessage("Main loop exit requested...", LOG_INFO);
    exitRequested = true;
}



/**
 * @brief Polls and handles all SDL_Events in the SDL Queue
 */
void handleEvents(void) noexcept
{
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_QUIT:
        {
            requestExit();
            break;
        }

        case SDL_DROPFILE:
        {
            std::filesystem::path file_path = event.drop.file;
            loadEmuSystem(file_path);
            break;
        }

        case SDL_KEYDOWN:
        {
            handleKeyboard(event.key);
            break;
        }
        }
    }
}



/**
 * @brief Handles a keypress
 * @param key
 */
void handleKeyboard(SDL_KeyboardEvent key)
{
    switch(key.keysym.scancode)
    {
    // Esc, pause/resume
    case SDL_SCANCODE_ESCAPE:
    {
        if(emuSystem != nullptr)
        {
            emuSystem->togglePause();
        }
        break;
    }

    // F3, step instruction
    case SDL_SCANCODE_F3:
    {
        if(emuSystem != nullptr && emuSystem->isPaused())
        {
            try
            {
                emuSystem->step(true);
            } catch(std::exception& ex)
            {
                logMessage(ex.what(), LOG_ERRORS);
            }
            
        }
        break;
    }

    // F5, step frame
    case SDL_SCANCODE_F5:
    {
        if(emuSystem != nullptr && emuSystem->isPaused())
        {
            try
            {
                emuSystem->resume();
                emuSystem->runFrame();
                emuSystem->pause();
            } catch(std::exception& ex)
            {
                logMessage(ex.what(), LOG_ERRORS);
            }
        }
        break;
    }

    // F9, resume
    case SDL_SCANCODE_F9:
    {
        if(emuSystem != nullptr && emuSystem->isPaused())
        {
            emuSystem->resume();
        }
        break;
    }

    default: break;
    }
}



/**
 * @brief Gets the key in a key/value pair
 * @param pair Key/Value pair
 * @param delimiter Key/Value separator
 * @throws std::invalid_argument if delimiter not found.
 */
std::string getKey(const std::string& pair, char delimiter)
{
    size_t delim_index = pair.find_first_of(delimiter);

    if(delim_index == std::string::npos)
    {
        std::string error_msg = fmt::format(
            "Cannot find delimiter {} in string {}!",
            delimiter,
            pair
        );

        throw std::invalid_argument(error_msg);
    }

    return pair.substr(0, delim_index);
}



/**
 * @brief Gets the value in a key/value pair
 * @param pair Key/Value pair
 * @param delimiter Key/Value separator
 * @throws std::invalid_argument if delimiter not found.
 */
std::string getValue(const std::string& pair, char delimiter)
{
    size_t delim_index = pair.find_first_of(delimiter);

    if(delim_index == std::string::npos)
    {
        std::string error_msg = fmt::format(
            "Cannot find delimiter {} in string {}!",
            delimiter,
            pair
        );

        throw std::invalid_argument(error_msg);
    }

    return pair.substr(delim_index + 1, pair.length());
}



/**
 * @brief Creates the emuSystem object, if it doesn't already exist.
 */
void createEmuSystem(void) noexcept
{
    if(emuSystem == nullptr) { emuSystem = new EmuSys(); }
}



/**
 * @brief Attempts to open a ROM in the emulated system.
 * @param file_path
 */
void loadEmuSystem(const std::filesystem::path file_path) noexcept
{
    if(!std::filesystem::exists(file_path))
    {
        logMessage(
            fmt::format("Couldn't load file {}.", file_path.string()),
            LOG_ERRORS
        );
    }

    try
    {
        createEmuSystem();
        emuSystem->stop();
        emuSystem->loadROM(file_path);
        emuSystem->start();
    } catch(std::exception& ex)
    {
        logMessage(fmt::format(
            "Couldn't load file {}. Error: {}",
            file_path.string(), ex.what()
        ),
            LOG_ERRORS
        );
    }
}
