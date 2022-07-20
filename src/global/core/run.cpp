#include "core/run.h"

// verdict implementation

bool verdict::isCriticalError() const {
    switch (val) {
        case TESTS_READ_ERROR:
        case GENERATOR_FAILED:
        case TO_TEST_FAILED:
        case PRIME_FAILED:
        case VERIFIER_FAILED:
        case GENERATOR_RE:
        case PRIME_RE:
        case VERIFIER_RE:
        case VERIFICATION_ERROR:
            return true;
        default:
            return false;
    }
}

bool verdict::isOrdinaryError() const {
    switch (val) {
        case WRONG_ANSWER:
        case RUNTIME_ERROR:
        case TIME_LIMIT:
        case MEMORY_LIMIT:
        case PRESENTATION_ERROR:
            return true;
        default:
            return false;
    }
}

std::string verdict::toString() const {
    switch (val) {
        case NOT_TESTED:
            return "Interrupted";
        case SKIPPED:
            return "Skipped by limits"; // todo: rephrase
        case TESTS_READ_ERROR:
            return "Reading file with tests failed";
        case GENERATOR_FAILED:
            return "Failed to run generator";
        case TO_TEST_FAILED:
            return "Failed to run solution";
        case PRIME_FAILED:
            return "Failed to run prime solution";
        case VERIFIER_FAILED:
            return "Failed to run verifier";
        case GENERATOR_RE:
            return "GENERATOR RE";
        case PRIME_RE:
            return "PRIME SOLUTION RE";
        case VERIFIER_RE:
            return "VERIFIER RE";
        case ACCEPTED:
            return "OK";
        case WRONG_ANSWER:
            return "Wrong answer";
        case RUNTIME_ERROR:
            return "Runtime error";
        case TIME_LIMIT:
            return "Time limit exceeded";
        case MEMORY_LIMIT:
            return "Memory limit exceeded";
        case VERIFICATION_ERROR:
            return "Verification error";
        case PRESENTATION_ERROR:
            return "Presentation error";
        default:
            return "Unknown";
    }
}

std::string verdict::toShortString() const {
    switch (val) {
        case PRIME_RE:
            return "RE of prime solution";
        case ACCEPTED:
            return "OK";
        case WRONG_ANSWER:
            return "WA";
        case RUNTIME_ERROR:
            return "RE";
        case TIME_LIMIT:
            return "TL";
        case MEMORY_LIMIT:
            return "ML";
        case PRESENTATION_ERROR:
            return "PE";
        default:
            return toString();
    }
}

// execution_error implementation

void execution_error::storeErrCode(size_t code) {
    errCode = code;
}

void execution_error::storeExitCode(size_t code) {
    exitCode = code;
}

bool execution_error::hasError() const {
    return exitCode | errCode;
}

bool execution_error::hasErrorInfo() const {
    return info != nullptr;
}

std::string execution_error::errorExplanation() const {
    std::stringstream str;

    if (errCode) {
        str << errInfoExplanation();
    } else {
        str << "non-zero exit code (" << std::hex;
        str << std::showbase << exitCode << ")";
    }
    return str.str();
}

void execution_error::clear() {
    exitCode = 0;
    errCode = 0;
    errInfoClear();
}

execution_error::~execution_error() {
    errInfoClear();
}

// test_result implementation

void test_result::clear() {
    input.clear();
    output.clear();
    output2.clear();
    err.clear();
    verdict = verdict::ACCEPTED;
    execResult.error.clear();
}
