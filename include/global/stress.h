#pragma once

#include <iostream>
#include <mutex>
#include "core/runtime_config.h"
#include "terminal.h"

namespace stress {

    // enter stress-tester
    void start(int, char *[]);

    // print usage
    void usage();
}
