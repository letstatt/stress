#include "core/run.h"
#include "win/core/error_info.h"

namespace {
    std::string errorCodeExplanation(size_t errCode) {
        const static std::string unknown = "unknown error";
        const static std::unordered_map<size_t, const std::string> m = {
                {EXCEPTION_ACCESS_VIOLATION,      "access violation"},
                {EXCEPTION_STACK_OVERFLOW,        "stack overflow"},
                {EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "array index out of bounds"},
                {EXCEPTION_ILLEGAL_INSTRUCTION,   "illegal instruction"},
                {EXCEPTION_INT_DIVIDE_BY_ZERO,    "division by zero"},
                {EXCEPTION_FLT_DIVIDE_BY_ZERO,    "division by zero"},
                {EXCEPTION_PRIV_INSTRUCTION,      "private instruction execution"}
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

void execution_error::storeErrInfo(error_info const& err) {
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
