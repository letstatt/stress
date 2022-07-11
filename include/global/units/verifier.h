#pragma once

#include "unit.h"

namespace units {
    class verifier : public unit {
    public:
        verifier(struct proto_unit const &u);

        void execute(runtime_config &cfg, test_result &test) override;

    private:
        bool verify(runtime_config &cfg, std::string const &a, std::string const &b);

        bool verifyTolerant(std::string const &a, std::string const &b);

        bool verifyStrict(std::string const &a, std::string const &b);
    };

}
