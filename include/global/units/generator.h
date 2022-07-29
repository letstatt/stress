#pragma once

#include "unit.h"
#include "core/tests_source.h"
#include <fstream>
#include <mutex>

// forward declaration
struct test_result;

namespace units {

// TODO: tag-dispatching (?)
    class generator : public unit {
        tests_source cat;
        std::ifstream reader;
        std::filesystem::directory_iterator iter;
        std::filesystem::directory_iterator iter_end{};
        std::mutex mutex;

        bool readNextTestFromFile(std::string &);
        bool readNextTestFromDir(std::string &);

    public:

        generator(tests_source testsSource, proto_unit const &u);

        bool prepare(runtime_config &cfg) override;

        void execute(runtime_config &, test_result &) override;
    };
}
