#pragma once

#include <string>
#include <filesystem>

// forward declaration
struct runtime_config;
struct test_result;

namespace units {

    enum class unit_category {
        TESTS_SOURCE = 0,
        TO_TEST = 1,
        PRIME = 2,
        VERIFIER = 3,
    };

    struct proto_unit {
        using path = std::filesystem::path;

        explicit proto_unit(unit_category cat) : cat(cat) {}

        // assertion that source exists
        void requireExistence() const;

        // return unit category string representation
        virtual std::string category() const;

        // return category + path
        std::string toString() const;

        // cut extension
        std::string filename() const;

        // check if unit has no source
        bool empty() const;

        virtual ~proto_unit() = default;

        path file;
        unit_category cat;
        size_t timeLimit = 0;
        size_t memoryLimit = 0;
    };

    struct unit : proto_unit {

        explicit unit(unit_category cat) : proto_unit(cat) {}
        explicit unit(struct proto_unit const& u) : proto_unit(u) {}

        // compile unit
        virtual bool prepare(runtime_config &cfg);

        // execute unit
        virtual void execute(runtime_config&, test_result&) = 0;
    };
}
