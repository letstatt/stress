#include "core/run.h"
#include "win/core/error_info.h"
#include <unordered_map>

namespace {
    std::string errorCodeExplanation(size_t errCode) {
        const static std::string unknown = "unknown error ";
        const static std::unordered_map<size_t, const std::string> m = {
                {EXCEPTION_ACCESS_VIOLATION,      "access violation"},
                {EXCEPTION_STACK_OVERFLOW,        "stack overflow"},
                {EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "array index out of bounds"},
                {EXCEPTION_ILLEGAL_INSTRUCTION,   "illegal instruction"},
                {EXCEPTION_FLT_DENORMAL_OPERAND,  "floating-point denormal operand"},
                {EXCEPTION_FLT_DIVIDE_BY_ZERO,    "floating-point division by zero"},
                {EXCEPTION_FLT_INEXACT_RESULT,    "floating-point inexact result"},
                {EXCEPTION_FLT_INVALID_OPERATION, "floating-point invalid operation"},
                {EXCEPTION_FLT_OVERFLOW,          "floating-point overflow"},
                {EXCEPTION_FLT_STACK_CHECK,       "stack overflowed or underflowed because of floating-point operation"},
                {EXCEPTION_FLT_UNDERFLOW,         "floating-point underflow"},
                {EXCEPTION_INT_DIVIDE_BY_ZERO,    "integer division by zero"},
                {EXCEPTION_INT_OVERFLOW,          "integer overflow"},
                {EXCEPTION_PRIV_INSTRUCTION,      "private instruction execution"},
                {EXCEPTION_GUARD_PAGE,            "guard page access attempt"},
                {EXCEPTION_IN_PAGE_ERROR,         "unable to load the page"},
                {EXCEPTION_PRIV_INSTRUCTION,      "privileged instruction"},
                {EXCEPTION_DATATYPE_MISALIGNMENT, "access to misaligned data"}
        };
        return (m.count(errCode) ? m.find(errCode)->second : unknown + std::to_string(errCode));
    }
}

// execution_error windows implementation

std::string execution_error::errInfoExplanation() const {
    std::string errorCodeExpl = errorCodeExplanation(errCode);

    if (info == nullptr) {
        return errorCodeExpl;
    } else {
        std::string errorInfoExpl = info->explanation();

        if (errorInfoExpl.empty()) {
            return errorCodeExpl;
        } else {
            return errorInfoExpl;
        }
    }
}

void execution_error::storeErrInfo(error_info const &err) {
    if (info) {
        errInfoClear();
    }
    info = new error_info(err);
}

void execution_error::errInfoClear() {
    if (info != nullptr) {
        delete info;
        info = nullptr;
    }
}
