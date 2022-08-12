#include "units/tests_source.h"
#include "core/run.h"
#include "core/error.h"
#include "invoker.h"

namespace units {

    // tests_source implementation

    tests_source::tests_source(tests_source_type testsSource, const struct proto_unit &u) : unit(u) {
        if ((type = testsSource) == tests_source_type::UNSPECIFIED) {
            throw error("Source of tests is unspecified");
        }
    }

    bool tests_source::readNextTestFromFile(std::string &test) {
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

    bool tests_source::readNextTestFromDir(std::string &test) {
        // go to the least file
        while (iter != iter_end && !iter->is_regular_file()) ++iter;
        if (iter == iter_end) {
            return true;
        }
        reader.close();
        reader.open(iter->path(), std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
        if (reader.bad()) {
            return false;
        }
        std::ifstream::pos_type pos = reader.tellg();
        std::vector<char> result(pos);

        reader.seekg(0, std::ios::beg);
        reader.read(&result[0], pos);

        if (reader.bad()) {
            return false;
        }
        test.append(result.begin(), result.end());

        // go to next file
        ++iter;
        return true;
    }

    std::string tests_source::category() const {
        using types = tests_source_type;

        switch (type) {
            case types::DIR:
                return "directory with tests";
            case types::EXECUTABLE:
                return "generator";
            case types::FILE:
                return "file with tests";
            case types::UNSPECIFIED:
                return "source of tests";
        }
    }

    bool tests_source::prepare(runtime_config &cfg) {
        if (type == tests_source_type::EXECUTABLE) {
            return unit::prepare(cfg);

        } else if (type == tests_source_type::FILE) {
            requireExistence();
            reader.open(file, std::ios_base::in | std::ios_base::binary);
            reader.unsetf(std::ios_base::skipws);

        } else if (type == tests_source_type::DIR) {
            if (!std::filesystem::is_directory(file)) {
                throw error(toString() + " is not a directory");
            }
            iter = std::filesystem::directory_iterator(file);
        }
        return true;
    }

    void tests_source::execute(runtime_config &cfg, test_result &test) {
        if (type == tests_source_type::EXECUTABLE) {
            const std::string seed = std::to_string(test.seed);
            if (!invoker::execute(cfg, *this, seed, test.input, test.err, test.execResult)) {
                test.verdict = verdict::GENERATOR_FAILED;
            }
            if (test.execResult.error.hasError()) {
                test.verdict = verdict::GENERATOR_RE;
            }
        } else if (type == tests_source_type::FILE) {
            std::lock_guard lck(mutex);
            if (!readNextTestFromFile(test.input)) {
                test.verdict = verdict::TESTS_READ_ERROR;
            }
            if (test.input.empty()) {
                test.verdict = verdict::TESTS_OVER;
            }
        } else if (type == tests_source_type::DIR) {
            std::lock_guard lck(mutex);
            if (!readNextTestFromDir(test.input)) {
                test.verdict = verdict::TESTS_READ_ERROR;
            }
            if (test.input.empty()) {
                test.verdict = verdict::TESTS_OVER;
            }
        }
    }
}
