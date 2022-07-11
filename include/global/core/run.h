#pragma once

#include <string>
#include "session.h"

class verdict {
public:
    enum value {
        NOT_TESTED,
        SKIPPED,
        TESTS_OVER,
        TESTS_READ_ERROR,
        GENERATOR_FAILED,
        TO_TEST_FAILED,
        PRIME_FAILED,
        VERIFIER_FAILED,
        GENERATOR_RE,
        PRIME_RE,
        VERIFIER_RE,
        ACCEPTED,
        WRONG_ANSWER,
        RUNTIME_ERROR,
        TIME_LIMIT,
        MEMORY_LIMIT,
        VERIFICATION_ERROR,
        PRESENTATION_ERROR
    };

    constexpr verdict() = default;

    constexpr verdict(value const &v) : val(v) {}

    bool isCriticalError() const;

    bool isOrdinaryError() const;

    std::string toString() const;

    std::string toShortString() const;

    // OPERATORS

    constexpr explicit operator value() const {
        return val;
    }

    explicit operator bool() = delete;

    constexpr bool operator==(verdict const &v) const {
        return val == v.val;
    }

    constexpr bool operator!=(verdict const &v) const {
        return val != v.val;
    }

private:
    value val = NOT_TESTED;
};

// os-dependent, must be implemented
struct error_info;

struct execution_error {

    void storeErrCode(size_t code);

    void storeExitCode(size_t code);

    // os-dependent
    void storeErrInfo(error_info const&);

    bool hasError() const;

    std::string errorExplanation() const;

    void clear();

    ~execution_error();

private:
    error_info* info = nullptr;
    size_t exitCode = 0;
    size_t errCode = 0;

    // os-dependent
    std::string errInfoExplanation() const;

    void errInfoClear();
};

struct execution_result {
    size_t memory = 0;
    size_t time = 0;
    execution_error error;
};

struct test_result {
    decltype(session::rand.operator()()) seed;
    execution_result execResult;
    verdict verdict;
    std::string input;
    std::string output;
    std::string output2;
    std::string err;

    void clear();
};
