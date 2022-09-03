#pragma once
#include "core/runtime_config.h"

namespace args {

    void init();

    runtime_config parseArgs(int argc, char*argv[]);

    void parseArgs(runtime_config& cfg, std::vector<std::string>& args, bool nested);

    void parseConfig(runtime_config& cfg, std::filesystem::path const& path);
}
