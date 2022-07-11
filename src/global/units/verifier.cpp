#include "units/verifier.h"
#include "core/runtime_config.h"
#include "core/run.h"
#include "invoker.h"
#include <algorithm>
#include <cstring>

namespace {
    char WHITESPACES[] = {'\t', '\r', '\n', ' '};

    bool isWhitespace(char c) {
        char* begin = std::begin(WHITESPACES);
        char* end = std::end(WHITESPACES);
        return std::find(begin, end, c) != end;
    }
}

namespace units {

    // verifier implementation

    verifier::verifier(proto_unit const &u) : unit(u) {}

    void verifier::execute(runtime_config &cfg, test_result &test) {
        if (empty()) {
            test.verdict =
                    verify(cfg, test.output, test.output2) ?
                    verdict::ACCEPTED : verdict::WRONG_ANSWER;
        } else {
            const std::string input = test.input + "\n" + test.output;

            if (!invoker::execute(cfg, *this, input, test.output2, test.err, test.execResult)) {
                test.verdict = verdict::VERIFIER_FAILED;

            } else if (test.execResult.error.hasError()) {
                test.verdict = verdict::VERIFIER_RE;

            } else if (test.output2.size() < 2) {
                test.verdict = verdict::VERIFICATION_ERROR;

            } else {
                std::string_view view = std::string_view(test.output2).substr(0, 2);

                if (view == "OK" || view == "AC") {
                    test.verdict = verdict::ACCEPTED;
                } else if (view == "WA") {
                    test.verdict = verdict::WRONG_ANSWER;
                } else if (view == "PE") {
                    test.verdict = verdict::PRESENTATION_ERROR;
                } else {
                    test.verdict = verdict::VERIFICATION_ERROR;
                }
            }
        }
    }

    bool verifier::verify(runtime_config &cfg, const std::string &a, const std::string &b) {
        if (cfg.strictVerifier) {
            return verifyStrict(a, b);
        } else {
            return verifyTolerant(a, b);
        }
    }

    bool verifier::verifyTolerant(const std::string &a, const std::string &b) {
        size_t pos1 = 0;
        size_t pos2 = 0;

        // skip whitespaces
        while (pos1 < a.size() && isWhitespace(a[pos1])) ++pos1;
        while (pos2 < b.size() && isWhitespace(b[pos2])) ++pos2;

        while (pos1 < a.size() && pos2 < b.size()) {
            bool w1 = isWhitespace(a[pos1]);
            bool w2 = isWhitespace(b[pos2]);

            if (w1 ^ w2) {
                // one reached end of the word, but second not
                return false;
            }
            if (w1 && w2) {
                // skip whitespaces together
                while (pos1 < a.size() && isWhitespace(a[pos1])) ++pos1;
                while (pos2 < b.size() && isWhitespace(b[pos2])) ++pos2;
                continue;
            }
            if (a[pos1] != b[pos2]) {
                return false;
            }
            ++pos1;
            ++pos2;
        }

        // last chance to trim whitespaces
        while (pos1 < a.size() && isWhitespace(a[pos1])) ++pos1;
        while (pos2 < b.size() && isWhitespace(b[pos2])) ++pos2;

        // if they both reached end, then no characters left
        return (pos1 == a.size() && pos2 == b.size());
    }

    bool verifier::verifyStrict(std::string const& a, std::string const& b) {
        if (a.size() != b.size()) {
            return false;
        }
        return memcmp(a.c_str(), b.c_str(), a.size()) == 0;
    }
}
