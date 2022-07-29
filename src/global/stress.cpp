#include "stress.h"
#include "parsing/args.h"
#include "logger.h"
#include "terminal.h"
#include "core/run.h"
#include "units/to_test.h"
#include "units/prime.h"
#include "units/verifier.h"
#include "invoker.h"
#include <sstream>
#include <vector>
#include <filesystem>
#include <optional>
#include <thread>

namespace fs = std::filesystem;

void dispatcher(runtime_config &, logger &);

void build_units(runtime_config &);

void worker(runtime_config &, session &);

void stress::start(int argc, char *argv[]) {
    runtime_config cfg = args::parseArgs(argc, argv);

    // todo: is it needed?
    try {
        cfg.workingDirectory = fs::current_path();
    } catch (...) {
        throw std::runtime_error("[!] Unable to get current working directory");
    }

    // ctor throws exception
    logger l(cfg);

    // if succeeded, clear screen
    terminal::clear();

    // initialize invoker
    invoker::initializer();

    // build units
    build_units(cfg);

    // compile and check
    for (const auto &[cat, unit]: cfg.units) {
        if (unit->empty()) continue;

        // is it needed?
        //unit->file = fs::canonical(unit->file);

        if (!unit->prepare(cfg)) {
            if (!terminal::interrupted()) {
                throw std::runtime_error("[!] Unable to prepare " + unit->category());
            } else {
                terminal::syncOutput("Stress-testing interrupted\n");
                return;
            }
        }
    }

    dispatcher(cfg, l);
}

void stress::usage() {
    const static std::string ABOUT = "stress v1.0 by letstatt";
    const static std::string SYNTAX = "Syntax:\nstress tests [options] to_test [prime]";

    const static std::pair<std::string, std::string> OPTIONS[] = {
            {"General:",   ""},
            {"-p",         "Pause each time test fails"},
            {"-c [gvtp]",  "Do not recompile files if compiled ones cached"},
            {"-n n",       "Run n tests (default: 10)\n"},
            {"Tests:",     ""},
            {"-g file",    "Path to test generator"},
            {"-t file",    "Path to file with tests"},
            {"-s seed",    "Start generator with a specific seed\n"},
            {"Limits:",    ""},
            {"-st",        "Display time and peak memory statistics"},
            {"-tl ms",     "Set time limit in milliseconds"},
            {"-ml mb",     "Set memory limit in MB\n"},
            {"Prime:",     ""},
            {"-ptl ms",    "Set time limit for prime"},
            {"-pml mb",    "Set memory limit for prime"},
            {"-pre",       "Do not stop if prime got RE\n"},
            {"Threading:", ""},
            {"-mt",        "Allow multithreaded testing"},
            {"-w n",       "Set count of workers\n"},
            {"Logging:",   ""},
            {"-stderr",    "Log stderr on runtime errors (useful with Java, Python, etc.)"},
            {"-tag",       "Set tag of log file"},
            {"-dnl",       "Disable logger (do not log)\n"},
            {"Verifier:",  ""},
            {"-v file",    "Path to custom verifier"},
            {"-vstrict",   "Use strict comparison of outputs\n"},
            {"Misc:",      ""},
            {"-cv",        "Collapse identical verdicts\n"}
    };

    std::cout << std::endl << ABOUT << std::endl;
    std::cout << std::endl << SYNTAX << std::endl << std::endl;

    size_t maxLength = 0;

    for (auto &i: OPTIONS) {
        maxLength = std::max(maxLength, i.first.length());
    }

    for (auto &i: OPTIONS) {
        std::cout << i.first << std::string(maxLength - i.first.length() + 3, ' ');
        std::cout << i.second << '\n';
    }
}

void build_units(runtime_config &cfg) {
    cfg.units.emplace(
            units::unit_category::GENERATOR,
            std::dynamic_pointer_cast<units::unit>(
                    std::make_shared<units::generator>(
                            cfg.useFileWithTests,
                            cfg.generator)));
    cfg.units.emplace(
            units::unit_category::TO_TEST,
            std::dynamic_pointer_cast<units::unit>(
                    std::make_shared<units::to_test>(
                            cfg.toTest)));
    cfg.units.emplace(
            units::unit_category::PRIME,
            std::dynamic_pointer_cast<units::unit>(
                    std::make_shared<units::prime>(
                            cfg.prime)));
    cfg.units.emplace(
            units::unit_category::VERIFIER,
            std::dynamic_pointer_cast<units::unit>(
                    std::make_shared<units::verifier>(
                            cfg.verifier)));
}

void dispatcher(runtime_config &cfg, logger &logger) {
    const auto hc = std::thread::hardware_concurrency();

    // keep one thread for stress process
    const auto idealThreadsCount = std::max(2u, hc ? hc - 1 : hc);

    // workers count shouldn't be more than tasks count
    const auto workersCount =
            std::min(cfg.testsCount,
                     cfg.multithreading ?
                     (cfg.workersCount ? cfg.workersCount : idealThreadsCount) : 1);

    std::vector<std::thread> workers(workersCount);
    session session(cfg, logger);

    if (cfg.multithreading) {
        terminal::syncOutput("[*] Workers count: ", workersCount, '\n');
    }
    terminal::syncOutput("[*] Ready\n\n");

    using namespace std::chrono;
    auto start = steady_clock::now();
    uint64_t elapsed;

    for (auto &i: workers) {
        i = std::thread(worker, std::ref(cfg), std::ref(session));
    }

    for (auto &i: workers) {
        i.join();
    }

    elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();

    if (cfg.displayStats) {
        std::stringstream stream;
        stream << '\n';
        stream << "Average time: " << (session.totalTime / session.testsDone) << " ms\n";
        stream << "Maximum time: " << session.maxTime << " ms\n";
        stream << "Completed in: " << elapsed << " ms\n";
        terminal::syncOutput(stream.str());
    }

    terminal::syncOutput('\n', "[*] Stress-testing is over. Solution is ");
    terminal::syncOutput(session.solutionBroken ? "broken" : "correct", '\n');
    terminal::syncOutput(logger.info(), '\n');
    terminal::flush();
}

void worker(runtime_config &cfg, session &session) {
    using cat = units::unit_category;

    test_result result;
    std::shared_ptr<units::generator> generator =
            std::dynamic_pointer_cast<units::generator>(
                    cfg.units[cat::GENERATOR]);

    // make run sequence
    std::vector<std::shared_ptr<units::unit>> u;
    u.emplace_back(cfg.units[cat::GENERATOR]);
    u.emplace_back(cfg.units[cat::TO_TEST]);

    if (!cfg.prime.empty()) {
        u.emplace_back(cfg.units[cat::PRIME]);
    }

    if (!cfg.verifier.empty() || !cfg.prime.empty()) {
        u.emplace_back(cfg.units[cat::VERIFIER]);
    }

    // run sequence n times in sum
    while (session.newTest(result.seed)) {
        for (auto &p: u) {
            p->execute(cfg, result);

            // if interrupted, there are no interesting errors
            if (terminal::interrupted()) {
                result.verdict = verdict::NOT_TESTED;
                session.processedTest(result);
                break;
            }

            // if error happened
            if (result.verdict.isOrdinaryError() || result.verdict.isCriticalError()) {
                session.processedTest(result);
                break;
            }

            // no more tests
            if (result.verdict == verdict::TESTS_OVER) {
                break;
            }
        }

        // if it's all OK
        if (result.verdict == verdict::ACCEPTED) {
            session.processedTest(result);
        }

        // clear struct before next run
        result.clear();
    }
}
