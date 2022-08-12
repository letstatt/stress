#include "terminal.h"
#include "core/error.h"
#include <windows.h>
#include <conio.h>
#include <io.h>
#include <csignal>

// terminal_token implementation

namespace {
    DWORD oldConsoleOutMode;
    DWORD oldConsoleInMode;
    HANDLE hOut;
    HANDLE hIn;
}

// This is a HandlerRoutine.
// When the signal is received, the system
// creates a new thread in the process to execute this function.

BOOL WINAPI ctrlHandler(DWORD) {
    terminal_utils::interruptionHandler(0); // dirty
    return TRUE;
}

void terminal_token::init() {
    // Illegal storage access
    // Other signals are useless on Windows
    signal(SIGSEGV, &terminal_utils::interruptionHandler);

    // Add another once ctrl+c handler
    if (!SetConsoleCtrlHandler(ctrlHandler, TRUE)) {
        throw error("Unable to set ctrl handler");
    }

    // Enable virtual terminal processing
    if ((hOut = CreateFile(
            "CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)) == INVALID_HANDLE_VALUE) {
        throw error("Unable to get console output handle, error " + std::to_string(GetLastError()));

    } else if (!GetConsoleMode(hOut, &oldConsoleOutMode)) {
        throw error("Unable to get console mode, error " + std::to_string(GetLastError()));

    } else if (!SetConsoleMode(
            hOut, oldConsoleOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        throw error("Unable to set console mode, error " + std::to_string(GetLastError()));
    }

    // Get console input buffer and disable echo
    if ((hIn = CreateFile(
            "CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)) == INVALID_HANDLE_VALUE) {
        throw error("Unable to get console input handle, error " + std::to_string(GetLastError()));

    } else if (!GetConsoleMode(hIn, &oldConsoleInMode)) {
        throw error("Unable to get console input mode, error " + std::to_string(GetLastError()));

    } else if (!SetConsoleMode(
            hIn, oldConsoleInMode & ~ENABLE_LINE_INPUT & ~ENABLE_ECHO_INPUT & ~ENABLE_PROCESSED_INPUT)) {
        throw error("Unable to set console input mode, error " + std::to_string(GetLastError()));
    }
}

terminal_token::~terminal_token() {
    SetConsoleMode(hOut, oldConsoleOutMode);
    SetConsoleMode(hIn, oldConsoleInMode);
}

// terminal implementation

int terminal::getChar() noexcept {
    FlushConsoleInputBuffer(hIn);

    // microsoft docs lies, _getch() successfully catches ctrl+c
    int ch = 0;
    while (ch == 0 || ch == 224) { // skip arrows
        ch = _getch();
    }
    return ch;
}

void terminal::safePrint(const char *str, size_t len) noexcept {
    WriteFile(hOut, str, len, nullptr, nullptr);
}

bool terminal::isStdinRedirected() noexcept {
    return (_isatty(_fileno(stdin)) == 0);
}

bool terminal::isStdoutRedirected() noexcept {
    return (_isatty(_fileno(stdout)) == 0);
}
