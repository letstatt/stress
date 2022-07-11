#include "core/runtime_config.h"
#include "core/session.h"
#include "terminal.h"
#include "logger.h"
#include "core/run.h"

// session implementation

session::session(runtime_config &cfg, class logger &logger) :
        cfg(cfg),
        logger(logger),
        rand(cfg.initialSeed) {}

bool session::newTest(decltype(rand.operator()()) &seed) {
    std::lock_guard lck(mutex);
    if (cancelled || testsStarted + 1 > cfg.testsCount) {
        return false;
    }
    ++testsStarted;
    seed = rand();
    return true;
}

void session::processedTest(test_result &result) {
    std::lock_guard lck(mutex);

    if (cancelled) {
        return;
    }

    if (result.verdict == verdict::TESTS_OVER) {
        testsStarted = cfg.testsCount;
        return;
    }

    uint32_t testId = ++testsDone;
    totalTime += result.execResult.time;
    maxTime = std::max(maxTime, result.execResult.time);

    // write a result to terminal
    terminal::writeTestResult(cfg, result, testId);

    // check if cancellation flag needs to be set
    if (terminal::interrupted()) {
        cancelled = true;
    }

    auto processExpl = [](auto status) -> void {
        if (status != logger::STATUS::OK && status != logger::STATUS::NOT_NEEDED) {
            terminal::syncOutput(logger::statusExplanation(status), '\n');
        }
    };

    if (result.verdict.isCriticalError()) {
        if (result.verdict == verdict::PRIME_RE) {
            auto status = logger.writeTest(cfg, result, testId);
            processExpl(status);
        }

        // critical error happened
        if (!(result.verdict == verdict::PRIME_RE && cfg.ignorePRE)) {
            cancel();
        }

    } else if (result.verdict.isOrdinaryError()) {
        auto status = logger.writeTest(cfg, result, testId, cfg.pausing);
        processExpl(status);
        solutionBroken = true;
        if (cfg.pausing && !cancelled) {
            terminal::pause();
        }
    }
}

void session::cancel() {
    terminal::interrupt();
    cancelled = true;
}
