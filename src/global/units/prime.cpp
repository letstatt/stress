#include "units/prime.h"
#include "core/runtime_config.h"
#include "core/run.h"
#include "invoker.h"

namespace units {

    // prime implementation

    prime::prime(proto_unit const& u) : unit(u) {}

    void prime::execute(runtime_config &cfg, test_result &test) {
        if (!invoker::execute(cfg, *this, test.input, test.output2, test.err, test.execResult)) {
            test.verdict = verdict::PRIME_FAILED;
        }
        else if (test.execResult.error.hasError()) {
            test.verdict = verdict::PRIME_RE;
        }
        else if (cfg.prime.timeLimit != 0 && test.execResult.time > cfg.prime.timeLimit) {
            test.verdict = verdict::SKIPPED;
        }
        else if (cfg.prime.memoryLimit != 0 && test.execResult.memory > cfg.prime.memoryLimit) {
            test.verdict = verdict::SKIPPED;
        }
    }

}
