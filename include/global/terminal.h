#pragma once

#include <iostream>
#include <atomic>
#include <mutex>

namespace terminal_utils {
    constexpr char hint[] = "(press any key to continue or Ctrl+C to exit)";

    void interruptionHandler(int sig) noexcept;
}

class terminal_token {
public:
    inline terminal_token() {
        if (instantiated) {
            throw std::runtime_error(
                    "terminal token has already been instantiated");
        }
        init();
        instantiated = true;
        valid = true;
    }

    terminal_token(terminal_token const &) = delete;

    inline terminal_token(terminal_token &&t) noexcept {
        std::swap(valid, t.valid);
    }

    ~terminal_token();

private:
    static void init();

    bool valid = false;
    inline static bool instantiated = false;
};

// forward declaration
struct runtime_config;
struct test_result;

class terminal {
public:
    static void clear() noexcept;

    static void writeTestResult(runtime_config const&, test_result &, uint32_t);

    static void pause() noexcept;

    static void printTerminationMessage() noexcept;

    static void interrupt() noexcept;

    static bool interrupted(std::memory_order m = std::memory_order_relaxed) noexcept;

    // write to stdout synchronously
    template<typename... Args>
    static void syncOutput(Args &&...args) {
        std::lock_guard lck(mutex);
        (std::cout << ... << args);
    }

    static void flush();

private:
    static int getChar() noexcept;

    static void safePrint(const char* str, size_t len) noexcept;

    inline static std::mutex mutex;
};
