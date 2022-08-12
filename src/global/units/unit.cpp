#include "units/unit.h"
#include "core/error.h"
#include "invoker.h"

namespace units {

    // proto_unit implementation

    void proto_unit::requireExistence() const {
        if (!std::filesystem::is_regular_file(file)) {
            throw error(toString() + " is not a regular file");
        }
    }

    std::string proto_unit::category() const {
        switch (cat) {
            case unit_category::TESTS_SOURCE:
                return "source of tests";
            case unit_category::TO_TEST:
                return "solution to test";
            case unit_category::PRIME:
                return "prime solution";
            case unit_category::VERIFIER:
                return "verifier";
            default:
                throw error("Unknown category");
        }
    }

    std::string proto_unit::toString() const {
        return category() + " (" + file.string() + ")";
    }

    std::string proto_unit::filename() const {
        return file.stem().string();
    }

    bool proto_unit::empty() const {
        return file.empty();
    }

    // unit implementation

    bool unit::prepare(runtime_config &cfg) {
        if (!invoker::isExecutable(file)) {
            if (!invoker::isCompilable(file)) {
                throw error("Don't know how to process " + toString());
            }

            if (!invoker::compile(cfg, *this)) {
                return false;
            }

            // compilation done there

            if (!invoker::isExecutable(file)) {
                throw error("Don't know how to process " + toString());
            }
        }
        requireExistence();
        return true;
    }
}
