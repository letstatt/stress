#include "win/core/error_info.h"

// error_info windows implementation

std::string error_info::explanation() const {
    if (empty) {
        return "";
    }

    if (address == 0) {
        return "nullptr dereference";
    }

    if (mbi.has_value()) {
        auto const& i = mbi.value();
        if (i.AllocationBase == nullptr) {
            return "segfault: page not mapped";
        }
    }

    switch (avType) {
        case WRITE:
            if (mbi.has_value()) {
                auto const& i = mbi.value();
                auto readonly = i.Protect & (PAGE_READONLY | PAGE_EXECUTE_READ);
                if (i.AllocationBase != nullptr && readonly) {
                    return "write to read-only memory";
                }
            }
            [[fallthrough]];
        case READ:
            return "segfault: memory permissions violation";
        case DEP:
            return "data execution attempt";
        default:
            return "unknown access violation, code " + std::to_string(avCode);
    }
}

void error_info::storeAccessViolation(ULONG type, ULONG addr) {
    switch (type) {
        case 0:
            avType = READ;
            break;
        case 1:
            avType = WRITE;
            break;
        case 8:
            avType = DEP;
            break;
        default:
            avType = UNKNOWN;
            break;
    }
    avCode = type;
    address = addr;
    empty = false;
}

void error_info::storeMemoryInfo(MEMORY_BASIC_INFORMATION info) {
    mbi = info;
}
