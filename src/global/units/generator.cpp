#include "units/generator.h"
#include "core/run.h"
#include "invoker.h"

namespace units {

    // generator implementation

    generator::generator(bool useFileWithTests, const struct proto_unit &u) : unit(u) {
        if (useFileWithTests) {
            cat = generator_category::READABLE;
            reader.open(file, std::ios_base::in | std::ios_base::binary);
            reader.unsetf(std::ios_base::skipws);
        } else {
            cat = generator_category::EXECUTABLE;
        }
    }

    bool generator::readTest(std::string &test) {
        size_t delimiters = 0;
        char c[2]{0}; // [1 prev_char, 0 cur_char]

        // trim
        while (!reader.eof()) {
            c[1] = c[0];
            if (!(reader >> c[0])) {
                break;
            }
            if (c[0] != '\r' && c[0] != '\n') {
                test += c[0];
                break;
            }
        }

        while (!reader.eof()) {
            c[1] = c[0];
            if (!(reader >> c[0])) { // todo: check correctness
                break;
            }

            if (c[0] == '\r') {
                ++delimiters;
                if (c[1] == '\r') {
                    break;
                }
                if (delimiters == 2) {
                    continue;
                }
            } else if (c[0] == '\n') {
                if (c[1] != '\r') {
                    ++delimiters;
                }
                if (delimiters == 2) {
                    break;
                }
            } else {
                if (delimiters == 2 && c[1] == '\r') {
                    test += c[1];
                }
                delimiters = 0;
            }
            test += c[0];
        }

        return reader.good() || reader.eof();
    }

    bool generator::prepare(runtime_config &cfg) {
        if (file.extension().string() == ".txt") {
            requireExistence();
            return true;
        } else {
            return unit::prepare(cfg);
        }
    }

    void generator::execute(runtime_config &cfg, test_result &test) {
        if (cat == generator_category::EXECUTABLE) {
            const std::string seed = std::to_string(test.seed);
            if (!invoker::execute(cfg, *this, seed, test.input, test.err, test.execResult)) {
                test.verdict = verdict::GENERATOR_FAILED;
            }
            if (test.execResult.error.hasError()) {
                test.verdict = verdict::GENERATOR_RE;
            }
        } else {
            std::lock_guard lck(mutex);
            if (!readTest(test.input)) {
                test.verdict = verdict::TESTS_READ_ERROR;
            }
            if (test.input.empty()) {
                test.verdict = verdict::TESTS_OVER;
            }
        }
    }
}
