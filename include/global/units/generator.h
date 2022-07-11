#pragma once

#include "unit.h"
#include <fstream>
#include <mutex>

// forward declaration
struct test_result;

namespace units {

// TODO: tag-dispatching
    class generator : public unit {
        enum class generator_category {
            EXECUTABLE,
            READABLE
        };

        generator_category cat;
        std::ifstream reader;
        std::mutex mutex;

        bool readTest(std::string &);

    public:

        generator(bool useFileWithTests, proto_unit const &u);

        bool prepare(runtime_config &cfg) override;

        void execute(runtime_config &, test_result &) override;
    };
}
