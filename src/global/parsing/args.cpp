#include "parsing/args.h"
#include "terminal.h"
#include "core/error.h"
#include <cstring>
#include <fstream>
#include <set>
#include <functional>

namespace {
    std::unordered_map<std::string, std::function<void(int&, std::vector<std::string>&, runtime_config&)>> arg_map;
    using cat = units::unit_category;

    auto parseUnsigned = [](int i, std::vector<std::string>& args, auto &result) {
        if (i >= args.size()) {
            throw error("Expected an unsigned integer");
        }
        try {
            std::string n{args[i]};
            for (auto &i: n) {
                if (!('0' <= i && i <= '9')) {
                    throw 0; // just go to catch(...)
                }
            }
            result = std::stoi(n);
        } catch (...) {
            throw error("Expected an unsigned integer");
        }
    };

    auto parsePath = [](int i, std::vector<std::string>& args, auto &result) {
        if (i >= args.size()) {
            throw error("Expected a path");
        }
        try {
            result = args[i];
        } catch (...) {
            throw error("Expected a path");
        }
        if (!exists(result)) {
            throw error(result.filename().string() + " doesn't exist");
        }
    };

    auto parseUnitCategory = [](int i, std::vector<std::string>& args, std::unordered_set<cat> &s) {
        if (i >= args.size()) {
            throw error("Expected a unit categories string");
        }
        const std::string tokens = args[i];
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

    auto parseTestsSource = [](int i, std::vector<std::string>& args, runtime_config& cfg, tests_source_type type) {
        if (cfg.testsSourceType != tests_source_type::UNSPECIFIED) {
            throw error("Only one tests source allowed");
        }
        parsePath(i, args, cfg.testsSource.file);
        cfg.testsSourceType = type;
    };
}

void args::init() {
    #define ARGHANDLER [](int& i, std::vector<std::string>& args, runtime_config& cfg) -> void
    #define ARG(name, func_body) arg_map.emplace(name, ARGHANDLER func_body);
    #define ARG_U(name, property) ARG(name, {parseUnsigned(++i, args, cfg.property);})
    #define ARG_TSRC(name, type) ARG(name, {parseTestsSource(++i, args, cfg, tests_source_type::type);});
    #define ARG_BOOL(name, property) ARG(name, {cfg.property = true;})

    // limits
    ARG_U("tl", timeLimit);
    ARG_U("ml", memoryLimit);
    ARG_U("ptl", primeTimeLimit);
    ARG_U("pml", primeMemoryLimit);

    // tests_config
    ARG_TSRC("g", EXECUTABLE);
    ARG_TSRC("f", FILE);
    ARG_TSRC("d", DIR);
    ARG_U("n", testsCount);
    ARG_U("s", initialSeed);

    // invoker_config
    ARG("w", { \
        parseUnsigned(++i, args, cfg.workersCount); \
        if (cfg.workersCount < 1) { \
            throw error("Count of workers must be positive"); \
        } \
    });
    ARG("c", {parseUnitCategory(++i, args, cfg.useCached);});
    ARG_BOOL("mt", multithreading);

    // terminal_config
    ARG_BOOL("cv", collapseVerdicts);
    ARG_BOOL("p", pausing);
    ARG_BOOL("st", displayStats);
    ARG_BOOL("dnl", doNotLog);
    
    // logger_config
    ARG_BOOL("stderr", logStderrOnRE);
    ARG("tag", { \
        if (++i >= args.size()) { \
            throw error("Expected a tag"); \
        } \
        cfg.tag = args[i]; \
    });

    // verifier_config
    ARG_BOOL("vstrict", strictVerifier);
    ARG("v", {parsePath(++i, args, cfg.verifier.file);});

    // load config
    // ?

    // runtime_config
    ARG_BOOL("pre", ignorePRE);
    ARG_BOOL("b", breaking);
}

runtime_config args::parseArgs(int argc, char* argv[]) {
    runtime_config cfg;

    std::vector<std::string> args(argc - 1);
    for (int i = 0; i < argc - 1; ++i) {
        *(args.begin() + i) =
            std::move(std::string(argv[i + 1]));
    }

    init();
    parseArgs(cfg, args, false);

    // checks
    if (cfg.testsSource.empty()) {
        throw error("Tests source was not set");

    } else if (cfg.toTest.empty()) {
        throw error("Solution to test was not set");

    } else if (!cfg.prime.empty() && !cfg.verifier.empty()) {
        throw error("Prime solution and verifier can only be set separately");

    } else if (cfg.pausing && cfg.breaking) {
        throw error("Pause and break flags can only be set separately");

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

void args::parseArgs(runtime_config& cfg, std::vector<std::string>& args, bool nested) {
    namespace fs = std::filesystem;

    std::set<std::string> repeating_arguments_checker;
    const static std::string cfg_flag = "-cfg";

    // check for duplicates and config loading
    for (int i = 0; i < args.size(); ++i) {
        if (args[i][0] == '-' && arg_map.count(args[i].substr(1))) {
            if (!repeating_arguments_checker.insert(args[i]).second) {
                throw error("Check for repeating options");
            }
        }

        if (args[i] == cfg_flag) {
            if (nested) {
                throw error("Recursive configs aren't supported");
            } else {
                fs::path p;
                try {
                    if (i == args.size()) {
                        throw 0; // jump to catch(...)
                    }
                    p = args[i + 1];
                } catch (...) {
                    throw error("Expected a path");
                }
                args::parseConfig(cfg, p);
            }
        }
    }

    // parse arguments
    for (int i = 0; i < args.size(); ++i) {
        if (args[i][0] == '-') {
            auto func = arg_map[args[i].substr(1)];

            if (func) {
                func(i, args, cfg);
            } else if (args[i] == cfg_flag) {
                ++i;
                continue;
            } else {
                throw error("Unknown option: " + args[i]);
            }

        } else if (cfg.toTest.empty()) {
            parsePath(i, args, cfg.toTest.file);

        } else if (cfg.prime.empty()) {
            parsePath(i, args, cfg.prime.file);

        } else {
            throw error("Unexpected token: " + args[i]);
        }
    }
    
}

void args::parseConfig(runtime_config& cfg, std::filesystem::path const& path) {
    if (!is_regular_file(path)) {
        throw error("Expected a file: " + path.string());
    }

    std::vector<std::string> args;
    std::string tmp;

    /* read config */
    std::ifstream reader(path);
    while (reader >> tmp) {
        args.push_back(tmp);
    }
    if (reader.bad()) {
        throw error("Unable to load config: " + path.string());
    }
    reader.close();

    try {
        parseArgs(cfg, args, true);

    } catch (error const& e) {
        std::stringstream str;
        str << "Config loading failed: ";
        str << path.string() << ", reason:" << std::endl;
        str << "    " << e.unformatted();
        throw error(str.str());
    }
}
