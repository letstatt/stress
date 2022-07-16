#include "core/run.h"
#include "linux/core/error_info.h"
#include <unordered_map>
#include <csignal>

namespace {
    std::string errorCodeExplanation(size_t errCode) {
        const static std::string unknown = "killed by signal ";
        const static std::unordered_map<unsigned long, const std::string> m = {
                {SIGSEGV, "segmentation fault"},
                {SIGABRT, "caught SIGABRT, mostly assertions/abort()"},
                {SIGFPE, "division by zero"}, // todo: not fair
                {SIGILL, "illegal instruction"}
        };
        return (m.count(errCode) ? m.find(errCode)->second : unknown + std::to_string(errCode));
    }
}

// execution_error linux implementation

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
