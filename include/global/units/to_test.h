#pragma once

#include "unit.h"

namespace units {

    class to_test : public unit {

    public:
        explicit to_test(struct proto_unit const& u);

        void execute(runtime_config &, test_result &);
    };
}
