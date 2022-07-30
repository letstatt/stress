#include "terminal.h"
#include "core/runtime_config.h"
#include "core/run.h"
#include <csignal>
#include <cstring>
#include <cmath>

namespace {
    constexpr char oops[] = "\nOops, unexpected termination.\nPlease, tell the developer how to reproduce this error.\n";
    constexpr char shutdown[] = "\nForce termination.\n";
    std::atomic<char const*> terminationMessage = oops;

    constexpr uint16_t MAX_INTERRUPTION_ATTEMPTS = 3;
    std::atomic_bool isInterrupted{false};
    uint16_t interruptionAttempts = 0;

    void forceTermination() {
        static std::atomic_flag termination; // clear state since C++20
        if (!termination.test_and_set()) {
            // enter branch only once.
            std::terminate();
        }
    }
}

void terminal_utils::interruptionHandler(int sig) noexcept {
    if (sig != 0 && sig != SIGINT) {
        // if not SIGINT, we can't properly continue, so die
        forceTermination();
    } else {
        // try to exit process manually.
        // if unsuccessful, kill it.
        isInterrupted.store(true, std::memory_order_relaxed);
        if (++interruptionAttempts > MAX_INTERRUPTION_ATTEMPTS) {
            terminationMessage.store(shutdown, std::memory_order_relaxed);
            forceTermination();
        }
    }
}

// terminal implementation

void terminal::clear() noexcept {
    // ESC[2J ESC[H - clear screen and move cursor to (0, 0)
    syncOutput("\x1B[2J\x1B[H");
}

void terminal::writeTestResult(runtime_config const &cfg, test_result &result, uint32_t testId) {
    const static int TEST_NUMBER_WIDTH = 3 + (int) std::floor(std::log10(cfg.testsCount));
    const static uint32_t VERDICT_WIDTH = 25;
    const static std::string CURSOR_UP = "\x1B[1A";

    static std::stringstream previousStream;
    static verdict previousVerdict = verdict::NOT_TESTED;
    static int32_t collapsedCounter = -1;

    std::stringstream str;
    bool collapsed;

    if (cfg.collapseVerdicts && previousVerdict == result.verdict && collapsedCounter >= 0) {
        str << CURSOR_UP << previousStream.str() << " (+" << ++collapsedCounter << ")";
        collapsed = true;

    } else {
        str << "Test " << std::setw(TEST_NUMBER_WIDTH);
        str << std::left << std::to_string(testId) + ", ";

        bool statsAllowed = false;

        if (!result.verdict.isCriticalError()
            || (result.verdict == verdict::PRIME_RE && cfg.ignorePRE)) { // todo: clean
            str << std::setw(VERDICT_WIDTH) << std::left << result.verdict.toString();
            statsAllowed = true;
        } else {
            str << result.verdict.toString();
        }

        if (cfg.displayStats && statsAllowed) {
            str << std::setw(4) << std::right << result.execResult.time << " ms, ";
            str << (result.execResult.memory / 1024 / 1024) << " MB";
        }
        collapsed = false;
    }

    syncOutput(str.str(), '\n');

    if (!collapsed) {
        previousStream = std::move(str);
        previousVerdict = result.verdict;
        collapsedCounter = 0;
    }
}

void terminal::pause() noexcept {
    syncOutput(terminal_utils::hint);

    int ch = getChar();
    if (ch == 3 || ch == 4) {
        // if ctrl+c or ctrl+d, interrupt
        interrupt();
    }
    // erase entire line and move cursor to the start
    syncOutput("\x1B[2K\r");
}

void terminal::printTerminationMessage() noexcept {
    char const* msg = terminationMessage.load();
    safePrint(msg, strlen(msg));
}

void terminal::interrupt() noexcept {
    isInterrupted.store(true, std::memory_order_relaxed);
}

bool terminal::interrupted(std::memory_order m) noexcept {
    return isInterrupted.load(m);
}

void terminal::flush() {
    std::lock_guard lck(mutex);
    std::cout.flush();
}

