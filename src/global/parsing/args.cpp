#include "parsing/args.h"
#include "terminal.h"
#include "core/error.h"
#include <cstring>

runtime_config args::parseArgs(int argc, char *argv[]) {
    namespace fs = std::filesystem;
    using cat = units::unit_category;
    runtime_config cfg;

    auto parseUnsigned = [&](int i, auto &result) {
        if (i == argc - 1) {
            throw error("Expected an unsigned integer");
        }
        try {
            result = std::stoi(argv[i + 1]);
        } catch (...) {
            throw error("Expected an unsigned integer");
        }
    };

    auto parsePath = [&](int i, fs::path &result) {
        if (i == argc - 1) {
            throw error("Expected a path");
        }
        try {
            result = argv[i + 1];
        } catch (...) {
            throw error("Expected a path");
        }
        if (!exists(result)) {
            throw error(result.filename().string() + " doesn't exist");
        }
    };

    auto parseUnitCategory = [&](int i, std::unordered_set<cat> &s) {
        if (i == argc - 1) {
            throw error("Expected a unit categories string");
        }
        const std::string tokens = argv[i + 1];
        const static std::unordered_map<char, cat> m = {
                {'g', cat::TESTS_SOURCE},
                {'v', cat::VERIFIER},
                {'t', cat::TO_TEST},
                {'p', cat::PRIME}
        };
        for (auto &ch: tokens) {
            auto it = m.find(ch);
            if (it != m.end()) {
                cat c = it->second;
                if (s.count(c)) {
                    throw error("Invalid unit categories string");
                }
                s.insert(c);
            } else {
                throw error("Expected a valid unit category");
            }
        }
    };

    // parse arguments
    for (int i = 1; i < argc; ++i) {
        // limits
        if (!strcmp(argv[i], "-tl")) {
            parseUnsigned(i++, cfg.timeLimit);

        } else if (!strcmp(argv[i], "-ml")) {
            parseUnsigned(i++, cfg.memoryLimit);

        } else if (!strcmp(argv[i], "-ptl")) {
            parseUnsigned(i++, cfg.primeTimeLimit);

        } else if (!strcmp(argv[i], "-pml")) {
            parseUnsigned(i++, cfg.primeMemoryLimit);
        }

        // tests_config
        else if (!strcmp(argv[i], "-g")) {
            switch (cfg.testsSourceType) {
                case tests_source_type::EXECUTABLE:
                    [[fallthrough]];
                case tests_source_type::UNSPECIFIED:
                    break;
                default:
                    throw error("Use only one source of tests");
            }
            parsePath(i++, cfg.testsSource.file);
            cfg.testsSourceType = tests_source_type::EXECUTABLE;

        } else if (!strcmp(argv[i], "-f")) {
            switch (cfg.testsSourceType) {
                case tests_source_type::FILE:
                    [[fallthrough]];
                case tests_source_type::UNSPECIFIED:
                    break;
                default:
                    throw error("Use only one source of tests");
            }
            parsePath(i++, cfg.testsSource.file);
            cfg.testsSourceType = tests_source_type::FILE;

        } else if (!strcmp(argv[i], "-d")) {
            switch (cfg.testsSourceType) {
                case tests_source_type::DIR:
                    [[fallthrough]];
                case tests_source_type::UNSPECIFIED:
                    break;
                default:
                    throw error("Use only one source of tests");
            }
            parsePath(i++, cfg.testsSource.file);
            cfg.testsSourceType = tests_source_type::DIR;

        } else if (!strcmp(argv[i], "-s")) {
            parseUnsigned(i++, cfg.initialSeed);
        }

        // invoker_config
        else if (!strcmp(argv[i], "-n")) {
            parseUnsigned(i++, cfg.testsCount);

        } else if (!strcmp(argv[i], "-w")) {
            parseUnsigned(i++, cfg.workersCount);
            if (cfg.workersCount < 1) {
                throw error("Count of workers must be a positive number");
            }

        } else if (!strcmp(argv[i], "-c")) {
            parseUnitCategory(i++, cfg.useCached);

        } else if (!strcmp(argv[i], "-mt")) {
            cfg.multithreading = true;
        }

        // terminal_config
        else if (!strcmp(argv[i], "-cv")) {
            cfg.collapseVerdicts = true;

        } else if (!strcmp(argv[i], "-p")) {
            cfg.pausing = true;

        } else if (!strcmp(argv[i], "-st")) {
            cfg.displayStats = true;

        } else if (!strcmp(argv[i], "-dnl")) {
            cfg.doNotLog = true;
        }

        // logger_config
        else if (!strcmp(argv[i], "-stderr")) {
            cfg.logStderrOnRE = true;

        } else if (!strcmp(argv[i], "-tag")) {
            if (i + 1 == argc) {
                throw error("Expected a tag");
            }
            cfg.tag = argv[++i];
        }

        // verifier_config
        else if (!strcmp(argv[i], "-vstrict")) {
            cfg.strictVerifier = true;

        } else if (!strcmp(argv[i], "-v")) {
            parsePath(i++, cfg.verifier.file);
        }

        // runtime_config
        else if (!strcmp(argv[i], "-pre")) {
            cfg.ignorePRE = true;

        } else if (argv[i][0] == '-') {
            throw error("Unknown option: " + std::string(argv[i]));

        } else if (cfg.toTest.empty()) {
            parsePath(i - 1, cfg.toTest.file);

        } else if (cfg.prime.empty()) {
            parsePath(i - 1, cfg.prime.file);

        } else {
            throw error("Unexpected token: " + std::string(argv[i]));
        }
    }

    // checks
    if (cfg.testsSource.empty()) {
        throw error("Source of tests was not set");

    } else if (cfg.toTest.empty()) {
        throw error("Solution to test was not set");

    } else if (!cfg.prime.empty() && !cfg.verifier.empty()) {
        throw error("Prime solution and verifier can only be set separately");
    } else if ((cfg.pausing || cfg.collapseVerdicts) && terminal::isStdoutRedirected()) {
        throw error("Flags -p and -cv cannot be set if stdout redirected");
    }

    // constraints check
    if ((cfg.timeLimit > 0 && cfg.timeLimit < constraints::MIN_TIME_LIMIT_MS)
    || (cfg.primeTimeLimit > 0 && cfg.primeTimeLimit < constraints::MIN_TIME_LIMIT_MS)) {
        throw error("Minimum time limit is " +
                std::to_string(constraints::MIN_TIME_LIMIT_MS) + " ms");
    }
    else if (cfg.memoryLimit > constraints::MAX_MEM_LIMIT_MB
    || cfg.primeMemoryLimit > constraints::MAX_MEM_LIMIT_MB) {
        throw error("Maximum memory limit is " +
                std::to_string(constraints::MAX_MEM_LIMIT_MB) + " MB");
    }

    // implicit configuring
    if (cfg.timeLimit > 0) {
        cfg.toTest.timeLimit = cfg.timeLimit;
        cfg.displayStats = true;
    }
    if (cfg.memoryLimit > 0) { // convert from MB to bytes
        cfg.toTest.memoryLimit = cfg.memoryLimit * 1024 * 1024;
        cfg.displayStats = true;
    }
    if (cfg.primeTimeLimit > 0) {
        cfg.prime.timeLimit = cfg.primeTimeLimit;
    }
    if (cfg.memoryLimit > 0) {
        cfg.prime.memoryLimit = cfg.primeMemoryLimit * 1024 * 1024;
    }
    if (cfg.initialSeed == 0) {
        using namespace std::chrono;
        cfg.initialSeed = duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()).count();
    }

    return cfg;
}
