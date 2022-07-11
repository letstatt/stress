#pragma once

#include <string>
#include <windows.h>
#undef min
#undef max

struct error_info {
    enum ACCESS_VIOLATION_TYPE {
        READ, WRITE, DEP, UNKNOWN
    };

    void storeAccessViolation(ULONG type, ULONG addr);

    std::string explanation() const;

private:
    bool empty = true;

    // store information about EXCEPTION_ACCESS_VIOLATION
    ACCESS_VIOLATION_TYPE avType = UNKNOWN;
    ULONG avCode{};
    ULONG address{};
};