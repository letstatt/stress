#pragma once

#include "unit.h"

namespace units {
    class prime : public unit {
    public:
        prime(struct proto_unit const& u);

        void execute(runtime_config &, test_result &) override;
    };

}
