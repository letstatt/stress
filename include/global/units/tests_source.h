#pragma once

#include "unit.h"
#include "core/tests_source_type.h"
#include <fstream>
#include <mutex>

// forward declaration
struct test_result;

namespace units {

// TODO: tag-dispatching (?)
    class tests_source : public unit {
        tests_source_type type;
        std::ifstream reader;
        std::filesystem::directory_iterator iter;
        std::filesystem::directory_iterator iter_end{};
        std::mutex mutex;

        bool readNextTestFromFile(std::string &);
        bool readNextTestFromDir(std::string &);

        std::string category() const override;

    public:

        tests_source(tests_source_type testsSource, proto_unit const &u);

        bool prepare(runtime_config &cfg) override;

        void execute(runtime_config &, test_result &) override;
    };
}
