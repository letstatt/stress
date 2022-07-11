#include "win/core/error_info.h"

// error_info windows implementation

std::string error_info::explanation() const {
    if (empty) {
        return "";
    }

    if (address == 0) {
        return "nullptr dereference";
    }

    switch (avType) {
        case READ:
            return "read from inaccessible memory";
        case WRITE:
            return "write to inaccessible memory";
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
