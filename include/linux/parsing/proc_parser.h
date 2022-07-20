#pragma once

#include "linux/core/maps.h"
#include <csignal>
#include <optional>

struct proc_parser {
    static std::optional<maps> get_maps(pid_t pid);
};
