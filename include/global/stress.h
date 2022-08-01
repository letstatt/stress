#pragma once

namespace build_info {
    extern const char * const version;
    extern const char * const build_number;
}

namespace stress {

    // enter stress-tester
    void start(int, char *[]);

    // print usage
    void usage();
}
