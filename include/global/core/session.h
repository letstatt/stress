#pragma once

#include "units/generator.h"
#include <random>

// forward declaration
struct runtime_config;

class terminal;

class logger;

struct session {
    uint64_t totalTime = 0;
    uint64_t maxTime = 0;
    uint32_t testsStarted = 0;
    uint32_t testsDone = 0;

    runtime_config const &cfg;
    logger &logger;
    std::mt19937 rand;
    std::mutex mutex;

    bool solutionBroken = false;
    bool cancelled = false; // if cancelled() -> terminal.interrupted()

    session(runtime_config &, class logger &);

    bool newTest(decltype(rand.operator()()) &seed);

    void processedTest(test_result &);

private:
    void cancel();
};