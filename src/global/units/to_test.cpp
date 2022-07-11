#include "units/to_test.h"
#include "core/runtime_config.h"
#include "core/run.h"
#include "invoker.h"

namespace units {

    to_test::to_test(const struct proto_unit &u) : unit(u) {}

    void to_test::execute(runtime_config &cfg, test_result &test) {
        if (!invoker::execute(cfg, *this, test.input, test.output, test.err, test.execResult)) {
            test.verdict = verdict::TO_TEST_FAILED;
        }
        else if (test.execResult.error.hasError()) {
            test.verdict = verdict::RUNTIME_ERROR;
        }
        else if (cfg.toTest.timeLimit != 0 && test.execResult.time > cfg.toTest.timeLimit) {
            test.verdict = verdict::TIME_LIMIT;
        }
        else if (cfg.toTest.memoryLimit != 0 && test.execResult.memory > cfg.toTest.memoryLimit) {
            test.verdict = verdict::MEMORY_LIMIT;
        }
        else {
            test.verdict = verdict::ACCEPTED;
        }
    }

}
