#pragma once
#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include "units/unit.h"

namespace constraints {
    constexpr uint32_t MIN_TIME_LIMIT_MS = 10;
    constexpr uint32_t MAX_MEM_LIMIT_MB = 4 * 1024;
}

struct limits {
    uint32_t timeLimit = 0;        // ms
    uint32_t memoryLimit = 0;      // bytes
    uint32_t primeTimeLimit = 0;   // ms
    uint32_t primeMemoryLimit = 0; // bytes
};

struct generator_config {
    uint32_t initialSeed = 0;
    bool useFileWithTests = false;
};

struct invoker_config {
    uint32_t testsCount = 10;
    uint32_t workersCount = 0;
    std::unordered_set<units::unit_category> useCached;
    bool multithreading = false;
};

struct terminal_config {
    bool collapseVerdicts = false;
    bool pausing = false;
    bool displayStats = false;
    bool doNotLog = false;
};

struct logger_config {
    bool logStderrOnRE = false;
    std::string tag;
};

struct verifier_config {
    bool strictVerifier = false;
};

struct runtime_config : limits, generator_config, invoker_config, terminal_config, logger_config, verifier_config {
    using path = std::filesystem::path;

    path workingDirectory;
    bool ignorePRE = false;

    units::proto_unit generator;
    units::proto_unit toTest;
    units::proto_unit prime;
    units::proto_unit verifier;

    std::unordered_map<
            units::unit_category,
            std::shared_ptr<units::unit>> units;

    runtime_config() :
            generator(units::unit_category::GENERATOR),
            toTest(units::unit_category::TO_TEST),
            prime(units::unit_category::PRIME),
            verifier(units::unit_category::VERIFIER) {}
};
