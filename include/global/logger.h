#pragma once

#include "core/runtime_config.h"
#include <fstream>

class logger {
public:
    enum class STATUS {
        NOT_NEEDED, OK, BAD, OPEN_ERROR, LOG_ERROR
    };

    explicit logger(runtime_config&);

    STATUS writeTest(runtime_config const&cfg, test_result const&, uint32_t, bool = false);

    static std::string statusExplanation(STATUS);

    std::string info();

private:
    std::ofstream logFile; // will be closed and freed in dtor
    std::filesystem::path path;
    bool bad = false;
};